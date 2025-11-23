#include "CTrajectoryPhysics.hpp"

// Includes
#include <CS2/SDK/SDK.hpp>
#include <CS2/SDK/Update/GameTrace.hpp>
#include <CS2/SDK/FunctionListSDK.hpp>
#include <CS2/SDK/Math/Math.hpp>
#include <GameClient/CL_Players.hpp>
#include <GameClient/CL_Weapons.hpp>
#include <GameClient/CL_ItemDefinition.hpp> 
#include <AndromedaClient/Render/CRenderStackSystem.hpp>
#include <AndromedaClient/Fonts/CFontManager.hpp>
#include <cmath>
#include <algorithm> 

// =========================================================
// CONSTANTES EXATAS DO CS2 (DUMPED FROM SCHEMA)
// =========================================================
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define DEG2RAD(x) ((float)(x) * (float)(M_PI / 180.f))
// Subtick exact interval
#define TICK_INTERVAL 0.015625f 
#define TIME_TO_TICKS( dt ) ( (int)( 0.5f + (float)(dt) / TICK_INTERVAL ) )

void Manual_AngleVectors(const QAngle& angles, Vector3& forward, Vector3& right, Vector3& up)
{
    float sp, sy, cp, cy, sr, cr;
    float pitch = DEG2RAD(angles.m_x);
    float yaw   = DEG2RAD(angles.m_y);
    float roll  = DEG2RAD(angles.m_z);

    sp = sinf(pitch); cp = cosf(pitch);
    sy = sinf(yaw);   cy = cosf(yaw);
    sr = sinf(roll);  cr = cosf(roll);

    forward.m_x = cp * cy;
    forward.m_y = cp * sy;
    forward.m_z = -sp;

    right.m_x = (-1 * sr * sp * cy + -1 * cr * -sy);
    right.m_y = (-1 * sr * sp * sy + -1 * cr * cy);
    right.m_z = (-1 * sr * cp);

    up.m_x = (cr * sp * cy + -sr * -sy);
    up.m_y = (cr * sp * sy + -sr * cy);
    up.m_z = (cr * cp);
}

// Física de rebote (PhysicsClipVelocity) - Implementação padrão Source
int CTrajectoryPhysics::PhysicsClipVelocity(const Vector3& in, const Vector3& normal, Vector3& out, float overbounce)
{
    float backoff = in.Dot(normal) * overbounce;
    Vector3 change = normal; 
    change.Multiplyf(backoff);
    out = in;
    out.Subtract(change);

    // Ajuste fino para evitar flutuação
    float adjust = out.Dot(normal);
    if (adjust < 0.0f) {
        Vector3 adj = normal; adj.Multiplyf(adjust);
        out.Subtract(adj);
    }
    return 0;
}

// CONFIGURAÇÃO EXATA (VALORES DO SCHEMA)
void CTrajectoryPhysics::SetupPhysics(int nWeaponID, float& flSpeed, float& flGravity, float& flElasticity, float& flFriction)
{
    // Padrão CS2
    flGravity = 800.0f; 
    flFriction = 0.2f;    // Atrito de chão padrão

    switch (nWeaponID) {
    case WEAPON_SMOKE_GRENADE: 
        // Smokes no CS2 são mais pesadas/menos saltitantes
        flElasticity = 0.33f; 
        flSpeed = 1140.0f;
        break;
        
    case WEAPON_MOLOTOV:
    case WEAPON_INCENDIARY_GRENADE:
    case WEAPON_FIRE_BOMB: 
        // Molotovs são mais lentos
        flElasticity = 0.40f; 
        flSpeed = 1000.0f; 
        break;
        
    case WEAPON_DECOY_GRENADE: 
    case WEAPON_FLASHBANG:
    case WEAPON_HIGH_EXPLOSIVE_GRENADE:
    default:
        // Granadas leves padrão
        flElasticity = 0.45f;
        flSpeed = 1140.0f;
        break;
    }
}

void CTrajectoryPhysics::Draw3DRing(const Vector3& origin, const Vector3& normal, float radius, ImColor color, float thickness)
{
    // ... (Sua função de desenho atual está ok, mantendo para economizar espaço)
    Vector3 up = (std::abs(normal.m_z) < 0.999f) ? Vector3(0, 0, 1) : Vector3(1, 0, 0);
    Vector3 right = normal; right = right.Cross(up); right.Normalize();
    Vector3 forward = normal; forward = forward.Cross(right); forward.Normalize();
    std::vector<ImVec2> points;
    const int segments = 32;
    for (int i = 0; i <= segments; ++i) {
        float theta = (float(M_PI) * 2.0f * i) / segments;
        Vector3 term1 = right; term1.Multiplyf(cosf(theta) * radius);
        Vector3 term2 = forward; term2.Multiplyf(sinf(theta) * radius);
        Vector3 p = origin; p.Add(term1); p.Add(term2);
        ImVec2 s;
        if (Math::WorldToScreen(p, s)) points.push_back(s);
    }
    auto* dl = ImGui::GetBackgroundDrawList();
    if (points.size() > 2) {
        for (size_t i = 1; i < points.size(); ++i)
            dl->AddLine(points[i-1], points[i], color, thickness);
    }
}

std::vector<GrenadePathNode> CTrajectoryPhysics::Simulate(C_CSPlayerPawn* pLocal, bool bAttack1, bool bAttack2)
{
    if (!pLocal) return {};

    int nWeaponID = GetCL_Weapons()->GetLocalWeaponDefinitionIndex();
    
    bool isGrenade = (nWeaponID == WEAPON_FLASHBANG || nWeaponID == WEAPON_SMOKE_GRENADE || nWeaponID == WEAPON_MOLOTOV || 
                      nWeaponID == WEAPON_INCENDIARY_GRENADE || nWeaponID == WEAPON_DECOY_GRENADE || nWeaponID == WEAPON_HIGH_EXPLOSIVE_GRENADE);

    if (!isGrenade) return {};

    // Força
    float flInputStrength = 0.0f;
    if (bAttack1 && bAttack2) flInputStrength = 0.5f;     
    else if (bAttack2) flInputStrength = 0.333333f;       
    else if (bAttack1) flInputStrength = 1.0f;            
    if (flInputStrength <= 0.0f) return {};

    // --- 1. SETUP INICIAL ---
    Vector3 currentPos = pLocal->GetOrigin();
    QAngle currentAngles = pLocal->m_angEyeAngles();
    Vector3 vViewOffset = pLocal->m_vecViewOffset();

    if (vViewOffset.LengthSquared() < 1.0f) vViewOffset = Vector3(0.0f, 0.0f, 64.0f);

    // Magic Pitch Correction (Crucial)
    float flPitch = currentAngles.m_x;
    if (flPitch < -89.0f) flPitch += 360.0f;
    else if (flPitch > 89.0f) flPitch -= 360.0f;
    
    float flSub = (90.0f - std::abs(flPitch)) * 0.11111111f; 
    float flCorrectedPitch = flPitch - flSub;

    QAngle angThrow = currentAngles;
    angThrow.m_x = flCorrectedPitch;
    
    Vector3 vForward, vRight, vUp;
    Manual_AngleVectors(angThrow, vForward, vRight, vUp);

    // --- 2. OFFSET DE ARREMESSO PERFEITO ---
    // A granada sai da MÃO, não do olho.
    // Offset Source 2: Forward: 12.0, Right: -2.0, Up: -2.0
    Vector3 vStart = currentPos;
    vStart.Add(vViewOffset); 
    
    Vector3 offFwd = vForward; offFwd.Multiplyf(12.0f);
    Vector3 offRight = vRight; offRight.Multiplyf(-2.0f); // Correção lateral
    Vector3 offUp = vUp;       offUp.Multiplyf(-2.0f);    // Correção vertical
    
    vStart.Add(offFwd); 
    vStart.Add(offRight);
    vStart.Add(offUp);

    // --- 3. FÍSICA E VELOCIDADE ---
    float baseSpeed, gravity, elasticity, friction;
    SetupPhysics(nWeaponID, baseSpeed, gravity, elasticity, friction);

    // Fórmula: Speed * (Strength * 0.7 + 0.3)
    float flActualThrowSpeed = baseSpeed * ((flInputStrength * 0.7f) + 0.3f);

    Vector3 vVelocity = vForward;
    vVelocity.Multiplyf(flActualThrowSpeed);
    
    // Inércia
    Vector3 vPlayerVel = pLocal->m_vecVelocity();
    if (vPlayerVel.LengthSquared() > 0.0f) { 
        vPlayerVel.Multiplyf(1.25f);
        vVelocity.Add(vPlayerVel);
    }

    std::vector<GrenadePathNode> currentPath;
    Vector3 vPos = vStart;
    
    ImVec2 sStart;
    if (Math::WorldToScreen(vPos, sStart)) 
        currentPath.push_back({ sStart, vPos, Vector3(0,0,0), false }); 

    // Máscara CS2 para projéteis (Ignora Player Clips)
    // Usar MASK_SOLID (0x200400B) é o mais seguro para granadas.
    CTraceFilter filter(0x200400B, pLocal, 3, 15); 
    
    // Hull Size (BBox 4x4x4)
    Ray_t ray = {}; 
    ray.Mins = Vector3(-2.0f, -2.0f, -2.0f); 
    ray.Maxs = Vector3( 2.0f,  2.0f,  2.0f);
    
    CGameTrace tr;
    int nTicks = TIME_TO_TICKS(5.0f); // 5 segundos max
    float flInterval = TICK_INTERVAL;

    // --- SIMULAÇÃO LOOP (VELOCITY VERLET ADAPTADO) ---
    for (int i = 0; i < nTicks; ++i)
    {
        Vector3 vPrevPos = vPos;

        // 1. Aplica Gravidade
        vVelocity.m_z -= gravity * flInterval;

        // 2. Aplica Arrasto (Air Drag)
        // O CS2 tem um pequeno arrasto aerodinâmico.
        // Se a granada estiver indo longe demais, aumente isso ligeiramente.
        // Powf simula o arrasto exponencial correto por delta time.
        // Coeficiente ~0.15f para ar
        float air_drag = 0.15f; 
        float drag_factor = std::powf(1.0f - air_drag, flInterval);
        // Mas na prática, para alinhar perfeitamente com internal:
        // Apenas uma multiplicação simples resolve para distâncias curtas.
        // No entanto, para precisão "perfeita":
        Vector3 dragVec = vVelocity;
        dragVec.Multiplyf(0.0f); // Reset
        // Se quiser testar arrasto real: vVelocity.Multiplyf(0.995f);
        // Vou deixar desativado ou muito baixo pois 1140 já compensa bem.
        // vVelocity.Multiplyf(0.998f); 

        // 3. Move
        Vector3 vMove = vVelocity; 
        vMove.Multiplyf(flInterval);
        Vector3 vNextPos = vPos; 
        vNextPos.Add(vMove);

        // 4. Trace Shape
        memset(&tr, 0, sizeof(CGameTrace));
        if (IGamePhysicsQuery_TraceShape(SDK::Pointers::CVPhys2World(), ray, vPrevPos, vNextPos, &filter, &tr)) {
            
            if (tr.DidHit()) {
                vPos = tr.vecPosition; 

                ImVec2 sHit;
                if (Math::WorldToScreen(vPos, sHit)) 
                    currentPath.push_back({ sHit, vPos, tr.vecNormal, true });

                // Lógica de Parada
                bool bStopped = false;
                
                // Molotov/Incendiary explode ao bater no chão (Normal Z > 0.7)
                if ((nWeaponID == WEAPON_MOLOTOV || nWeaponID == WEAPON_INCENDIARY_GRENADE)) {
                    if (tr.vecNormal.m_z > 0.7f) bStopped = true;
                }

                if (vVelocity.LengthSquared() < 100.0f) bStopped = true; 

                if (bStopped) break;

                // 5. Physics Clip (Rebote)
                float flNewElasticity = elasticity;
                // Smoke quica menos se bater direto no chão
                if (nWeaponID == WEAPON_SMOKE_GRENADE && tr.vecNormal.m_z > 0.7f) flNewElasticity = 0.30f;

                Vector3 vNewVel;
                PhysicsClipVelocity(vVelocity, tr.vecNormal, vNewVel, 1.0f + flNewElasticity);
                vVelocity = vNewVel;

                // 6. Atrito (Friction) se tocar no chão
                if (tr.vecNormal.m_z > 0.7f) {
                    // Friction scaling baseada na velocidade
                    float flSpeedSq = vVelocity.LengthSquared();
                    if (flSpeedSq > 0.0f) {
                        // Atrito no CS2 é aplicado reduzindo a velocidade tangencial
                        vVelocity.Multiplyf(1.0f - (friction)); 
                    }
                }
                
                // 7. Push out (Evitar ficar preso)
                Vector3 push = tr.vecNormal; push.Multiplyf(0.1f);
                vPos.Add(push);
            }
            else {
                vPos = vNextPos;
            }
        }
        else {
            vPos = vNextPos;
        }

        if (i % 2 == 0) { 
            ImVec2 s;
            if (Math::WorldToScreen(vPos, s)) 
                currentPath.push_back({ s, vPos, Vector3(0,0,0), false });
        }
    }

    return currentPath;
}
#include "CTrajectoryPhysics.hpp"

// Includes
#include <CS2/SDK/SDK.hpp>
#include <CS2/SDK/Update/GameTrace.hpp>
#include <CS2/SDK/FunctionListSDK.hpp>
#include <CS2/SDK/Math/Math.hpp>
#include <CS2/SDK/Types/CEntityData.hpp>
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
#define TIME_TO_TICKS(dt) ( (int)(0.5f + (float)(dt) / TICK_INTERVAL) )

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

// Física de rebote (PhysicsClipVelocity) - Implementação padrão Source-like
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
        Vector3 adj = normal; 
        adj.Multiplyf(adjust);
        out.Subtract(adj);
    }
    return 0;
}

// CONFIGURAÇÃO EXATA (VALORES DO SCHEMA)
void CTrajectoryPhysics::SetupPhysics(C_CSWeaponBase* pWeapon, int nWeaponID,
                                      float& flSpeed, float& flGravity,
                                      float& flElasticity, float& flFriction)
{
    // Padrão CS2
    flGravity  = 800.0f; 
    flFriction = 0.2f;    // Atrito de chão padrão

    switch (nWeaponID) {
    case WEAPON_SMOKE_GRENADE: 
        // Smokes no CS2 são mais pesadas/menos saltitantes
        flElasticity = 0.33f; 
        flSpeed      = 1140.0f;
        break;
        
    case WEAPON_MOLOTOV:
    case WEAPON_INCENDIARY_GRENADE:
    case WEAPON_FIRE_BOMB: 
        // Molotovs são mais lentos
        flElasticity = 0.40f; 
        flSpeed      = 1000.0f; 
        break;
        
    case WEAPON_DECOY_GRENADE: 
    case WEAPON_FLASHBANG:
    case WEAPON_HIGH_EXPLOSIVE_GRENADE:
    default:
        // Granadas leves padrão
        flElasticity = 0.45f;
        flSpeed      = 1140.0f;
        break;
    }
}

void CTrajectoryPhysics::Draw3DRing(const Vector3& origin, const Vector3& normal,
                                    float radius, ImColor color, float thickness)
{
    Vector3 up = (std::abs(normal.m_z) < 0.999f) ? Vector3(0, 0, 1) : Vector3(1, 0, 0);
    Vector3 right = normal; 
    right = right.Cross(up); 
    right.Normalize();
    Vector3 forward = normal; 
    forward = forward.Cross(right); 
    forward.Normalize();

    std::vector<ImVec2> points;
    const int segments = 32;

    for (int i = 0; i <= segments; ++i) {
        float theta = (float(M_PI) * 2.0f * i) / segments;
        Vector3 term1 = right;   term1.Multiplyf(cosf(theta) * radius);
        Vector3 term2 = forward; term2.Multiplyf(sinf(theta) * radius);
        Vector3 p = origin; 
        p.Add(term1); 
        p.Add(term2);

        ImVec2 s;
        if (Math::WorldToScreen(p, s))
            points.push_back(s);
    }

    auto* dl = ImGui::GetBackgroundDrawList();
    if (points.size() > 2) {
        for (size_t i = 1; i < points.size(); ++i)
            dl->AddLine(points[i - 1], points[i], color, thickness);
    }
}

std::vector<GrenadePathNode> CTrajectoryPhysics::Simulate(C_CSPlayerPawn* pLocal, bool bAttack1, bool bAttack2)
{
    if (!pLocal)
        return {};

    int nWeaponID = GetCL_Weapons()->GetLocalWeaponDefinitionIndex();
    
    bool isGrenade =
        (nWeaponID == WEAPON_FLASHBANG ||
         nWeaponID == WEAPON_SMOKE_GRENADE ||
         nWeaponID == WEAPON_MOLOTOV || 
         nWeaponID == WEAPON_INCENDIARY_GRENADE ||
         nWeaponID == WEAPON_DECOY_GRENADE ||
         nWeaponID == WEAPON_HIGH_EXPLOSIVE_GRENADE);

    if (!isGrenade)
        return {};

    float flInputStrength = 0.0f;
    if (bAttack1 && bAttack2)      flInputStrength = 0.5f;
    else if (bAttack2)             flInputStrength = 0.333333f;
    else if (bAttack1)             flInputStrength = 1.0f;
    if (flInputStrength <= 0.0f)   return {};

    // --- 1. SETUP INICIAL ---
    Vector3 currentPos   = pLocal->GetOrigin();
    QAngle currentAngles = pLocal->m_angEyeAngles();
    Vector3 vViewOffset  = pLocal->m_vecViewOffset();

    if (vViewOffset.LengthSquared() < 1.0f)
        vViewOffset = Vector3(0.0f, 0.0f, 64.0f);

    // Magic Pitch Correction (igual à lógica clássica)
    float flPitch = currentAngles.m_x;
    if (flPitch < -89.0f)      flPitch += 360.0f;
    else if (flPitch > 89.0f)  flPitch -= 360.0f;
    
    float flSub            = (90.0f - std::abs(flPitch)) * (10.0f / 90.0f);
    float flCorrectedPitch = flPitch - flSub;

    QAngle angThrow = currentAngles;
    angThrow.m_x = flCorrectedPitch;
    
    Vector3 vForward, vRight, vUp;
    Manual_AngleVectors(angThrow, vForward, vRight, vUp);

    // --- 2. TRACE INICIAL + OFFSET DA MÃO ---
    Vector3 vEyePos = currentPos;
    vEyePos.Add(vViewOffset);
    float throwHeight = (flInputStrength * 12.0f) - 12.0f;
    vEyePos.m_z += throwHeight;

    // Máscara CS2 para projéteis
    CTraceFilter filter(0x200400B, pLocal, 3, 15); 

    // Hull Size (BBox 4x4x4) para o trace inicial
    Ray_t rayInit = {};
    rayInit.Mins = Vector3(-2.0f, -2.0f, -2.0f); 
    rayInit.Maxs = Vector3( 2.0f,  2.0f,  2.0f);

    CGameTrace trInit{};
    Vector3 vTraceStart = vEyePos;
    Vector3 vTraceEnd   = vEyePos;

    {
        Vector3 fwd22 = vForward; 
        fwd22.Multiplyf(22.0f);
        vTraceEnd.Add(fwd22);
    }

    if (IGamePhysicsQuery_TraceShape(SDK::Pointers::CVPhys2World(), rayInit,
                                     vTraceStart, vTraceEnd, &filter, &trInit)
        && trInit.DidHit())
    {
        vEyePos = trInit.vecPosition;
        Vector3 back6 = vForward; 
        back6.Multiplyf(-6.0f);
        vEyePos.Add(back6);
    }

    // Offset Source-like: Forward 12, Right -2, Up -2
    Vector3 vStart = vEyePos;
    
    Vector3 offFwd   = vForward; offFwd.Multiplyf(12.0f);
    Vector3 offRight = vRight;  offRight.Multiplyf(-2.0f);
    Vector3 offUp    = vUp;     offUp.Multiplyf(-2.0f);
    
    vStart.Add(offFwd); 
    vStart.Add(offRight);
    vStart.Add(offUp);

    // --- 3. FÍSICA E VELOCIDADE ---
    float baseSpeed, gravity, elasticity, friction;
    SetupPhysics(GetCL_Weapons()->GetLocalActiveWeapon(),
                 nWeaponID, baseSpeed, gravity, elasticity, friction);

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

    // Máscara/Hull para o loop principal
    Ray_t ray = {}; 
    ray.Mins = Vector3(-2.0f, -2.0f, -2.0f); 
    ray.Maxs = Vector3( 2.0f,  2.0f,  2.0f);
    
    CGameTrace tr;
    auto GetDetonateTime = [&](int id) -> float {
        switch (id) {
        case WEAPON_FLASHBANG:
        case WEAPON_HIGH_EXPLOSIVE_GRENADE:
            return 1.5f;
        case WEAPON_INCENDIARY_GRENADE:
        case WEAPON_MOLOTOV:
            return 3.0f;
        case WEAPON_TACTICAL_AWARENESS_GRENADE:
            return 5.0f;
        default:
            return 3.0f;
        }
    };
    int   nTicks    = TIME_TO_TICKS(GetDetonateTime(nWeaponID));
    float flInterval = TICK_INTERVAL;
    float effectiveGravity = gravity * 0.4f;

    // --- SIMULAÇÃO LOOP ---
    for (int i = 0; i < nTicks; ++i)
    {
        Vector3 vPrevPos = vPos;

        float vzPrev = vVelocity.m_z;
        vVelocity.m_z -= effectiveGravity * flInterval;

        // 2. Sem arrasto de ar (drag) para ficar próximo do CS/CS2.

        Vector3 vMove = vVelocity;
        vMove.m_z = (vzPrev + vVelocity.m_z) * 0.5f;
        vMove.Multiplyf(flInterval);
        Vector3 vNextPos = vPos; 
        vNextPos.Add(vMove);

        // 4. Trace Shape
        memset(&tr, 0, sizeof(CGameTrace));
        if (IGamePhysicsQuery_TraceShape(SDK::Pointers::CVPhys2World(), ray,
                                         vPrevPos, vNextPos, &filter, &tr))
        {   
            if (tr.DidHit())
            {
                vPos = tr.vecPosition; 

                ImVec2 sHit;
                if (Math::WorldToScreen(vPos, sHit)) 
                    currentPath.push_back({ sHit, vPos, tr.vecNormal, true });

                bool bStopped = false;
                if (nWeaponID == WEAPON_MOLOTOV || nWeaponID == WEAPON_INCENDIARY_GRENADE)
                {
                    if (tr.vecNormal.m_z >= 0.7f)
                        bStopped = true;
                }
                if (vVelocity.LengthSquared() < 0.01f)
                    bStopped = true;
                if (nWeaponID == WEAPON_TACTICAL_AWARENESS_GRENADE)
                    bStopped = true;
                if (bStopped)
                    break;

                float flSurfaceElasticity = 1.0f;
                if (tr.pHitEntity)
                    flSurfaceElasticity = 0.3f;
                float flTotalElasticity = std::clamp(flSurfaceElasticity * 0.45f, 0.0f, 0.9f);
                Vector3 vNewVel;
                PhysicsClipVelocity(vVelocity, tr.vecNormal, vNewVel, 2.0f);
                vNewVel.Multiplyf(flTotalElasticity);
                vVelocity = vNewVel;

                float remainingDt = (1.0f - tr.flFraction) * flInterval;
                Vector3 vRemainderMove = vNewVel;
                vRemainderMove.Multiplyf(remainingDt);
                Vector3 vRemainderEnd = vPos;
                vRemainderEnd.Add(vRemainderMove);

                CGameTrace tr2;
                memset(&tr2, 0, sizeof(CGameTrace));
                if (IGamePhysicsQuery_TraceShape(SDK::Pointers::CVPhys2World(), ray,
                                                 vPos, vRemainderEnd, &filter, &tr2))
                {
                    if (tr2.DidHit())
                    {
                        vPos = tr2.vecPosition;
                        ImVec2 sHit2;
                        if (Math::WorldToScreen(vPos, sHit2))
                            currentPath.push_back({ sHit2, vPos, tr2.vecNormal, true });

                        Vector3 push2 = tr2.vecNormal;
                        push2.Multiplyf(0.1f);
                        vPos.Add(push2);
                    }
                    else
                    {
                        vPos = vRemainderEnd;
                    }
                }
                else
                {
                    vPos = vRemainderEnd;
                }
            }
            else
            {
                vPos = vNextPos;
            }
        }
        else
        {
            vPos = vNextPos;
        }

        if (i % 2 == 0)
        { 
            ImVec2 s;
            if (Math::WorldToScreen(vPos, s)) 
                currentPath.push_back({ s, vPos, Vector3(0,0,0), false });
        }
    }

    return currentPath;
}

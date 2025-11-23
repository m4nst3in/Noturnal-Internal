#include "CTrajectoryPhysics.hpp"

// Includes da sua SDK e Helpers
#include <CS2/SDK/SDK.hpp>
#include <CS2/SDK/Update/GameTrace.hpp>
#include <CS2/SDK/FunctionListSDK.hpp>
#include <CS2/SDK/Math/Math.hpp>

// Seus sistemas
#include <GameClient/CL_Players.hpp>
#include <GameClient/CL_Weapons.hpp>
#include <GameClient/CL_ItemDefinition.hpp> 

// INCLUDE DO LOGGER (Verifique se o caminho está certo)
#include <Common/DevLog.hpp> 

#include <cmath>
#include <algorithm> 

// --- CONSTANTES ---
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define DEG2RAD(x) ((float)(x) * (float)(M_PI / 180.f))
#define TICK_INTERVAL 0.015625f 
#define TIME_TO_TICKS( dt ) ( (int)( 0.5f + (float)(dt) / TICK_INTERVAL ) )

// CS2 Grenade Physics Constants
constexpr float CS2_GRENADE_THROW_SPEED = 750.0f;      // Base throw speed
constexpr float CS2_GRENADE_GRAVITY = 800.0f;          // Gravity value for CS2
constexpr float CS2_GRENADE_ELASTICITY = 0.45f;        // Default bounce coefficient
constexpr float CS2_GRENADE_FRICTION = 0.2f;           // Velocity loss on ground contact

// --- MATEMÁTICA MANUAL ---
void Manual_AngleVectors(const QAngle& angles, Vector3& forward)
{
    float sp, sy, cp, cy;
    float pitch = DEG2RAD(angles.m_x);
    float yaw   = DEG2RAD(angles.m_y);

    sp = sinf(pitch);
    cp = cosf(pitch);
    sy = sinf(yaw);
    cy = cosf(yaw);

    forward.m_x = cp * cy;
    forward.m_y = cp * sy;
    forward.m_z = -sp;
}

int CTrajectoryPhysics::PhysicsClipVelocity(const Vector3& in, const Vector3& normal, Vector3& out, float overbounce)
{
    float backoff = in.Dot(normal) * overbounce;
    Vector3 change = normal; 
    change.Multiplyf(backoff);
    out = in;
    out.Subtract(change);

    if (out.m_x > -0.1f && out.m_x < 0.1f) out.m_x = 0.0f;
    if (out.m_y > -0.1f && out.m_y < 0.1f) out.m_y = 0.0f;
    if (out.m_z > -0.1f && out.m_z < 0.1f) out.m_z = 0.0f;

    float adjust = out.Dot(normal);
    if (adjust < 0.0f) {
        Vector3 adj = normal; adj.Multiplyf(adjust);
        out.Subtract(adj);
    }
    return 0;
}

void CTrajectoryPhysics::SetupPhysics(int nWeaponID, float& flSpeed, float& flGravity, float& flElasticity, float& flFriction)
{
    // CS2 correct physics constants
    flSpeed = CS2_GRENADE_THROW_SPEED; 
    flGravity = CS2_GRENADE_GRAVITY; 
    flElasticity = CS2_GRENADE_ELASTICITY; 
    flFriction = CS2_GRENADE_FRICTION;
    
    switch (nWeaponID) {
    case WEAPON_SMOKE_GRENADE: 
        flElasticity = 0.3f; 
        break;
    case WEAPON_MOLOTOV:
    case WEAPON_INCENDIARY_GRENADE:
    case WEAPON_FIRE_BOMB: 
        flSpeed = 750.0f; 
        flElasticity = 0.35f; 
        break;
    case WEAPON_DECOY_GRENADE: 
        flElasticity = 0.5f; 
        break;
    default: 
        break;
    }
}

void CTrajectoryPhysics::Draw3DRing(const Vector3& origin, const Vector3& normal, float radius, ImColor color, float thickness)
{
    Vector3 up = (std::abs(normal.m_z) < 0.999f) ? Vector3(0, 0, 1) : Vector3(1, 0, 0);
    Vector3 right = normal; right = right.Cross(up); right.Normalize();
    Vector3 forward = normal; forward = forward.Cross(right); forward.Normalize();

    std::vector<ImVec2> points;
    const int segments = 32;
    const float step = (float(M_PI) * 2.0f) / (float)segments;

    for (int i = 0; i <= segments; ++i) {
        float theta = i * step;
        Vector3 term1 = right; term1.Multiplyf(cosf(theta) * radius);
        Vector3 term2 = forward; term2.Multiplyf(sinf(theta) * radius);
        Vector3 pointWorld = origin; pointWorld.Add(term1); pointWorld.Add(term2);
        ImVec2 screenPos;
        if (Math::WorldToScreen(pointWorld, screenPos)) points.push_back(screenPos);
    }
    auto* drawList = ImGui::GetBackgroundDrawList();
    if (points.size() > 2) {
        for (size_t i = 1; i < points.size(); ++i)
            drawList->AddLine(points[i - 1], points[i], color, thickness);
    }
}

// TIMER DE DEBUG
static int g_DebugTimer = 0;

std::vector<GrenadePathNode> CTrajectoryPhysics::Simulate(C_CSPlayerPawn* pLocal, bool bAttack1, bool bAttack2)
{
    if (!pLocal) return {};

    int nWeaponID = GetCL_Weapons()->GetLocalWeaponDefinitionIndex();
    
    Vector3 currentPos = pLocal->GetOrigin();
    QAngle currentAngles = pLocal->m_angEyeAngles();

    bool isGrenade = (nWeaponID == WEAPON_FLASHBANG || nWeaponID == WEAPON_SMOKE_GRENADE || nWeaponID == WEAPON_MOLOTOV || 
                      nWeaponID == WEAPON_INCENDIARY_GRENADE || nWeaponID == WEAPON_DECOY_GRENADE || nWeaponID == WEAPON_HIGH_EXPLOSIVE_GRENADE);

    if (!isGrenade) return {};

    std::vector<GrenadePathNode> currentPath;
    
    // Calculate throw strength correctly
    // Full throw (left click only): 1.0f
    // Underhand throw (right click only): 0.5f  
    // Medium throw (both clicks): 0.5f
    float flThrowStrength = 1.0f;  // Default: full throw
    if (bAttack2 && !bAttack1) {
        flThrowStrength = 0.5f;  // Underhand throw
    } else if (bAttack1 && bAttack2) {
        flThrowStrength = 0.5f;  // Medium throw
    } else if (!bAttack1 && !bAttack2) {
        return {};  // No throw
    }
    
    if (flThrowStrength <= 0.0f) return {};

    float flPitch = currentAngles.m_x;
    if (flThrowStrength < 1.0f) {
        float flSub = (90.0f - std::abs(flPitch)) * 0.1111f;
        flPitch = (flPitch < 0.0f) ? flPitch - flSub : flPitch + flSub;
    }
    
    QAngle angThrow = currentAngles;
    angThrow.m_x = flPitch; 
    
    Vector3 vForward;
    Manual_AngleVectors(angThrow, vForward);

    // Leitura de ViewOffset
    Vector3 vViewOffset = pLocal->m_vecViewOffset();
    
    // Se o ViewOffset estiver zerado (bug de leitura), usa o padrão (0,0,64)
    if (vViewOffset.LengthSquared() < 1.0f) { 
        vViewOffset = Vector3(0.0f, 0.0f, 64.0f); 
    }

    Vector3 vStart = currentPos;
    vStart.Add(vViewOffset); 
    
    Vector3 offset = vForward; offset.Multiplyf(10.0f); 
    vStart.Add(offset);

    float speed, gravity, elasticity, friction;
    SetupPhysics(nWeaponID, speed, gravity, elasticity, friction);

    // Calculate initial velocity correctly (no 0.9f multiplier)
    Vector3 vVelocity = vForward;
    vVelocity.Multiplyf(speed * flThrowStrength);
    
    // --- LOG DE DEBUG ---
    // Imprime apenas a cada 60 chamadas (aprox. 1 vez por segundo)
    if (g_DebugTimer++ > 60) 
    {
        g_DebugTimer = 0;
        
        DEV_LOG("=== GRENADE DEBUG ===\n");
        DEV_LOG("Angles: Pitch=%.2f, Yaw=%.2f\n", currentAngles.m_x, currentAngles.m_y);
        DEV_LOG("Forward Vec: %.2f, %.2f, %.2f\n", vForward.m_x, vForward.m_y, vForward.m_z);
        DEV_LOG("Origin: %.2f, %.2f, %.2f\n", currentPos.m_x, currentPos.m_y, currentPos.m_z);
        DEV_LOG("ViewOffset: %.2f, %.2f, %.2f\n", vViewOffset.m_x, vViewOffset.m_y, vViewOffset.m_z);
        DEV_LOG("Start Pos: %.2f, %.2f, %.2f\n", vStart.m_x, vStart.m_y, vStart.m_z);
        DEV_LOG("Velocity: %.2f, %.2f, %.2f\n", vVelocity.m_x, vVelocity.m_y, vVelocity.m_z);
        DEV_LOG("=====================\n");
    }

    Vector3 vPos = vStart;
    ImVec2 sStart;
    if (Math::WorldToScreen(vPos, sStart)) currentPath.push_back({ sStart, vPos, Vector3(0,0,0), false }); 

    CTraceFilter filter(0x1C1003, pLocal, 3, 15); 
    Ray_t ray; CGameTrace tr;
    int nTicks = TIME_TO_TICKS(3.0f); float flInterval = TICK_INTERVAL;

    for (int i = 0; i < nTicks; ++i)
    {
        // Apply gravity ONCE per tick (before calculating movement)
        if (gravity != 0.0f) {
            vVelocity.m_z -= gravity * flInterval;
        }
        
        // Calculate next position
        Vector3 vMove = vVelocity; 
        vMove.Multiplyf(flInterval);
        Vector3 vNextPos = vPos; 
        vNextPos.Add(vMove);

        bool bHit = false;
        memset(&tr, 0, sizeof(CGameTrace));

        if (IGamePhysicsQuery_TraceShape(SDK::Pointers::CVPhys2World(), ray, vPos, vNextPos, &filter, &tr))
        {
            if (tr.DidHit())
            {
                bHit = true;
                vPos = tr.vecPosition;
                ImVec2 sHit;
                if (Math::WorldToScreen(vPos, sHit)) currentPath.push_back({ sHit, vPos, tr.vecNormal, true });

                // Molotov/Incendiary breaks on ground hit
                if ((nWeaponID == WEAPON_MOLOTOV || nWeaponID == WEAPON_INCENDIARY_GRENADE) && tr.vecNormal.m_z > 0.7f) break;

                // Calculate reflection using PhysicsClipVelocity
                Vector3 vNewVel;
                PhysicsClipVelocity(vVelocity, tr.vecNormal, vNewVel, 2.0f);
                vVelocity = vNewVel;
                
                // Apply elasticity (bounce coefficient)
                vVelocity.Multiplyf(elasticity);
                
                // Apply friction on horizontal surfaces
                if (tr.vecNormal.m_z > 0.7f) {
                    vVelocity.Multiplyf(1.0f - friction);
                }
                
                // Stop if velocity is too low
                float speed = vVelocity.Length();
                if (speed < 0.1f) break;
            }
            else vPos = vNextPos;
        }
        else vPos = vNextPos;

        if (!bHit && (i % 2 == 0)) {
            ImVec2 s;
            if (Math::WorldToScreen(vPos, s)) currentPath.push_back({ s, vPos, Vector3(0,0,0), false });
        }
    }
    return currentPath;
}
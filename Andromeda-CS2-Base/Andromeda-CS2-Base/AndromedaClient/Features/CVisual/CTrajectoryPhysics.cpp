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

#include <cmath>
#include <algorithm> 

// =========================================================
// CONFIGURAÇÕES
// =========================================================
#define TICK_INTERVAL           0.015625f 
#define TIME_TO_TICKS( dt )     ( (int)( 0.5f + (float)(dt) / TICK_INTERVAL ) )

// Cache Global
static std::vector<GrenadePathNode> g_CachedPath;
static QAngle g_LastAngles = { 0.f, 0.f, 0.f };
static Vector3 g_LastPos = { 0.f, 0.f, 0.f };
static int g_LastWeaponID = -1;
static bool g_LastAttack1 = false;
static bool g_LastAttack2 = false;

// =========================================================
// HELPERS LOCAIS
// =========================================================

// Calcula distância ao quadrado entre dois Vectors (Usando sua SDK)
float DistSqrVec(const Vector3& a, const Vector3& b) {
    Vector3 delta = a;
    delta.Subtract(b);
    return delta.LengthSquared();
}

// Calcula distância ao quadrado entre dois QAngles (Usando sua SDK)
float DistSqrAng(const QAngle& a, const QAngle& b) {
    QAngle delta = a;
    delta.Subtract(b);
    return delta.LengthSquared();
}

// =========================================================
// FÍSICA E HELPERS
// =========================================================

int CTrajectoryPhysics::PhysicsClipVelocity(const Vector3& in, const Vector3& normal, Vector3& out, float overbounce)
{
    float backoff = in.Dot(normal) * overbounce;

    // SDK: out = in - (normal * backoff)
    Vector3 change = normal; 
    change.Multiplyf(backoff);
    
    out = in;
    out.Subtract(change);

    // Correção de precisão
    if (out.m_x > -0.1f && out.m_x < 0.1f) out.m_x = 0.0f;
    if (out.m_y > -0.1f && out.m_y < 0.1f) out.m_y = 0.0f;
    if (out.m_z > -0.1f && out.m_z < 0.1f) out.m_z = 0.0f;

    float adjust = out.Dot(normal);
    if (adjust < 0.0f)
    {
        Vector3 adj = normal;
        adj.Multiplyf(adjust);
        out.Subtract(adj);
    }
    return 0;
}

void CTrajectoryPhysics::SetupPhysics(int nWeaponID, float& flSpeed, float& flGravity, float& flElasticity, float& flFriction)
{
    flSpeed = 1000.0f; flGravity = 800.0f; flElasticity = 0.45f; flFriction = 0.2f;      

    switch (nWeaponID)
    {
    case WEAPON_SMOKE_GRENADE: flElasticity = 0.3f; break;
    case WEAPON_MOLOTOV:
    case WEAPON_INCENDIARY_GRENADE:
    case WEAPON_FIRE_BOMB: flSpeed = 750.0f; flElasticity = 0.35f; break;
    case WEAPON_DECOY_GRENADE: flElasticity = 0.5f; break;
    default: break;
    }
}

void CTrajectoryPhysics::Draw3DRing(const Vector3& origin, const Vector3& normal, float radius, ImColor color, float thickness)
{
    Vector3 up = (std::abs(normal.m_z) < 0.999f) ? Vector3(0, 0, 1) : Vector3(1, 0, 0);
    
    // Cross product usando sua SDK
    Vector3 right = normal; 
    right = right.Cross(up); 
    right.Normalize();

    Vector3 forward = normal;
    forward = forward.Cross(right);
    forward.Normalize();

    std::vector<ImVec2> points;
    const int segments = 32;
    const float step = (3.14159265f * 2.0f) / (float)segments;

    for (int i = 0; i <= segments; ++i)
    {
        float theta = i * step;
        
        // pointWorld = origin + (right * cos) + (forward * sin)
        Vector3 term1 = right; term1.Multiplyf(cosf(theta) * radius);
        Vector3 term2 = forward; term2.Multiplyf(sinf(theta) * radius);
        
        Vector3 pointWorld = origin;
        pointWorld.Add(term1);
        pointWorld.Add(term2);

        ImVec2 screenPos;
        if (Math::WorldToScreen(pointWorld, screenPos)) points.push_back(screenPos);
    }

    auto* drawList = ImGui::GetBackgroundDrawList();
    if (points.size() > 2)
    {
        for (size_t i = 1; i < points.size(); ++i)
            drawList->AddLine(points[i - 1], points[i], color, thickness);
    }
}

// =========================================================
// SIMULAÇÃO
// =========================================================

std::vector<GrenadePathNode> CTrajectoryPhysics::Simulate(C_CSPlayerPawn* pLocal, bool bAttack1, bool bAttack2)
{
    if (!pLocal) return {};

    int nWeaponID = GetCL_Weapons()->GetLocalWeaponDefinitionIndex();
    QAngle currentAngles = pLocal->m_angEyeAngles();
    Vector3 currentPos = pLocal->m_vOldOrigin();

    bool isGrenade = (nWeaponID == WEAPON_FLASHBANG || nWeaponID == WEAPON_SMOKE_GRENADE || nWeaponID == WEAPON_MOLOTOV || 
                      nWeaponID == WEAPON_INCENDIARY_GRENADE || nWeaponID == WEAPON_DECOY_GRENADE || nWeaponID == WEAPON_HIGH_EXPLOSIVE_GRENADE);

    if (!isGrenade) return {};

    // Cache check
    if (!g_CachedPath.empty() && nWeaponID == g_LastWeaponID && bAttack1 == g_LastAttack1 && bAttack2 == g_LastAttack2 &&
        DistSqrVec(currentPos, g_LastPos) < 1.0f && DistSqrAng(currentAngles, g_LastAngles) < 0.05f)
    {
        return g_CachedPath;
    }

    std::vector<GrenadePathNode> currentPath;
    float flThrowStrength = (bAttack1 && bAttack2) ? 0.5f : (bAttack2 ? 0.3333f : (bAttack1 ? 1.0f : 0.0f));
    if (flThrowStrength == 0.0f) return {};

    float flPitch = currentAngles.m_x;
    if (flThrowStrength < 1.0f)
    {
        float flSub = (90.0f - std::abs(flPitch)) * 0.1111f;
        flPitch = (flPitch < 0.0f) ? flPitch - flSub : flPitch + flSub;
    }
    
    QAngle angThrow = currentAngles;
    angThrow.m_x = flPitch; 
    
    Vector3 vForward, vRight, vUp;
    Math::AngleVectors(angThrow, vForward, vRight, vUp);

    Vector3 vStart = currentPos;
    // vStart += ViewOffset
    vStart.Add(pLocal->m_vecViewOffset()); 
    
    // Matemática manual para offset de spawn (12, -2, -2)
    Vector3 fwd = vForward; fwd.Multiplyf(12.0f);
    Vector3 rgt = vRight;   rgt.Multiplyf(-2.0f);
    Vector3 upp = vUp;      upp.Multiplyf(-2.0f);
    
    vStart.Add(fwd); vStart.Add(rgt); vStart.Add(upp);

    float speed, gravity, elasticity, friction;
    SetupPhysics(nWeaponID, speed, gravity, elasticity, friction);

    // vVelocity = (vForward * (speed * 0.9f * flThrowStrength)) + (pLocal->m_vecVelocity() * 1.25f);
    Vector3 vVelocity = vForward;
    vVelocity.Multiplyf(speed * 0.9f * flThrowStrength);
    
    // Inércia (Agora vai funcionar pois adicionamos m_vecVelocity no CEntityData)
    Vector3 vInertia = pLocal->m_vecVelocity(); 
    vInertia.Multiplyf(1.25f);
    vVelocity.Add(vInertia);

    Vector3 vPos = vStart;
    ImVec2 sStart;
    if (Math::WorldToScreen(vPos, sStart)) currentPath.push_back({ sStart, vPos, Vector3(0,0,0), false }); 

    CTraceFilter filter(0x1C1003, pLocal, 3, 15); 
    Ray_t ray; CGameTrace tr;
    int nTicks = TIME_TO_TICKS(3.0f); float flInterval = TICK_INTERVAL;

    for (int i = 0; i < nTicks; ++i)
    {
        if (gravity != 0.0f) vVelocity.m_z -= (gravity * 0.5f) * flInterval;
        
        Vector3 vMove = vVelocity;
        vMove.Multiplyf(flInterval);
        
        Vector3 vNextPos = vPos;
        vNextPos.Add(vMove);

        if (gravity != 0.0f) vVelocity.m_z -= (gravity * 0.5f) * flInterval;

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

                if ((nWeaponID == WEAPON_MOLOTOV || nWeaponID == WEAPON_INCENDIARY_GRENADE) && tr.vecNormal.m_z > 0.7f) break;

                Vector3 vNewVel;
                PhysicsClipVelocity(vVelocity, tr.vecNormal, vNewVel, 1.0f + elasticity);
                vVelocity = vNewVel;

                if (tr.vecNormal.m_z > 0.7f) vVelocity.Multiplyf(1.0f - friction);
                if (vVelocity.LengthSquared() < 400.0f) break;
            }
            else vPos = vNextPos;
        }
        else vPos = vNextPos;

        if (!bHit && (i % 2 == 0))
        {
            ImVec2 s;
            if (Math::WorldToScreen(vPos, s)) currentPath.push_back({ s, vPos, Vector3(0,0,0), false });
        }
    }

    g_CachedPath = currentPath;
    g_LastAngles = currentAngles;
    g_LastPos = currentPos;
    g_LastWeaponID = nWeaponID;
    g_LastAttack1 = bAttack1;
    g_LastAttack2 = bAttack2;
    return currentPath;
}
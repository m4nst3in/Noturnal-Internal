#pragma once

#include <CS2/SDK/Math/Vector3.hpp>
#include <CS2/SDK/Math/QAngle.hpp>
#include <CS2/SDK/Math/Math.hpp> 
#include <ImGui/imgui.h>
#include <vector>

// Forward Declaration
class C_CSPlayerPawn;

struct GrenadePathNode
{
    ImVec2 m_ScreenPos;
    Vector3 m_WorldPos;
    Vector3 m_Normal;   
    bool m_bDidHit;     
};

class CTrajectoryPhysics
{
public:
    // Simulação Principal (STATIC)
    static std::vector<GrenadePathNode> Simulate(
        C_CSPlayerPawn* pLocal, 
        bool bAttack1, 
        bool bAttack2
    );

    // Desenho 3D (STATIC)
    static void Draw3DRing(
        const Vector3& origin, 
        const Vector3& normal, 
        float radius, 
        ImColor color, 
        float thickness = 2.0f
    );

private:
    // --- CORREÇÃO: Adicionado 'static' e removido 'flDrag' ---
    static void SetupPhysics(int nWeaponID, float& flSpeed, float& flGravity, float& flElasticity, float& flFriction);    
    
    // (STATIC)
    static int PhysicsClipVelocity(const Vector3& in, const Vector3& normal, Vector3& out, float overbounce);
};
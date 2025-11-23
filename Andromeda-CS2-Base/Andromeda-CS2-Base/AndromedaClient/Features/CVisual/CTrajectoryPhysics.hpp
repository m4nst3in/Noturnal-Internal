#pragma once

#include <CS2/SDK/Math/Vector3.hpp>
#include <CS2/SDK/Math/QAngle.hpp>
#include <CS2/SDK/Math/Math.hpp> 
#include <ImGui/imgui.h>
#include <vector>

// Forward Declarations (Essenciais para evitar o erro C2660/E0167)
class C_CSPlayerPawn;
class C_CSWeaponBase; 

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
    // Simulação Principal
    static std::vector<GrenadePathNode> Simulate(
        C_CSPlayerPawn* pLocal, 
        bool bAttack1, 
        bool bAttack2
    );

    // Desenho 3D
    static void Draw3DRing(
        const Vector3& origin, 
        const Vector3& normal, 
        float radius, 
        ImColor color, 
        float thickness = 2.0f
    );

private:
    // --- CORREÇÃO: Assinatura atualizada para receber o ponteiro da arma ---
    static void SetupPhysics(
        C_CSWeaponBase* pWeapon, 
        int nWeaponID, 
        float& flSpeed, 
        float& flGravity, 
        float& flElasticity, 
        float& flFriction
    );    
    
    static int PhysicsClipVelocity(const Vector3& in, const Vector3& normal, Vector3& out, float overbounce);
};
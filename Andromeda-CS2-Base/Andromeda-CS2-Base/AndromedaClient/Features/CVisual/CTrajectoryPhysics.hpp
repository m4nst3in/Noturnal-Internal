#pragma once

#include <CS2/SDK/Math/Vector3.hpp>
#include <CS2/SDK/Math/QAngle.hpp>
#include <ImGui/imgui.h>
#include <vector>

// Forward declarations
class C_CSPlayerPawn;
class C_CSWeaponBase; // Assumindo que você tem essa classe base

struct GrenadePathNode
{
    ImVec2 m_ScreenPos;
    Vector3 m_WorldPos;
    bool m_bOnGround;
};

class CTrajectoryPhysics
{
public:
    // Retorna a lista de pontos para desenhar
    // bAttack1 = Segurando Botão Esquerdo
    // bAttack2 = Segurando Botão Direito
    static std::vector<GrenadePathNode> Simulate(
        C_CSPlayerPawn* pLocal, 
        C_CSWeaponBase* pWeapon, 
        bool bAttack1, 
        bool bAttack2
    );

private:
    static void SetupPhysics(C_CSWeaponBase* pWeapon, float& flSpeed, float& flGravity, float& flElasticity, float& flFriction, float& flDrag);
    static int PhysicsClipVelocity(const Vector3& in, const Vector3& normal, Vector3& out, float overbounce);
};
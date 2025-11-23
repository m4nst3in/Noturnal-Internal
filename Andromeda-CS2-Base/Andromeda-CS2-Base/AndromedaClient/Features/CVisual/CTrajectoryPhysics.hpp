#pragma once

#include <CS2/SDK/Math/Vector3.hpp>
#include <CS2/SDK/Math/QAngle.hpp>
#include <CS2/SDK/Math/Math.hpp> // Para ImVec2 e WorldToScreen
#include <ImGui/imgui.h>
#include <vector>

// Forward Declaration
class C_CSPlayerPawn;

struct GrenadePathNode
{
    ImVec2 m_ScreenPos;
    Vector3 m_WorldPos;
    Vector3 m_Normal;   // Normal da superfície atingida (para desenhar o anel 3D)
    bool m_bDidHit;     // Se este ponto é um impacto
};

class CTrajectoryPhysics
{
public:
    // Retorna a lista de pontos para desenhar (com Cache e Física Avançada)
    static std::vector<GrenadePathNode> Simulate(
        C_CSPlayerPawn* pLocal, 
        bool bAttack1, 
        bool bAttack2
    );

    // Desenha um anel 3D alinhado à normal da superfície
    static void Draw3DRing(
        const Vector3& origin, 
        const Vector3& normal, 
        float radius, 
        ImColor color, 
        float thickness = 2.0f
    );

private:
    static void SetupPhysics(int nWeaponID, float& flSpeed, float& flGravity, float& flElasticity, float& flFriction);
    static int PhysicsClipVelocity(const Vector3& in, const Vector3& normal, Vector3& out, float overbounce);
};
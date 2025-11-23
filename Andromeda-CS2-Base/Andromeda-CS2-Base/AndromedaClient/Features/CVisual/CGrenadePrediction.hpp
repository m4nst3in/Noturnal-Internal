#pragma once

#include <vector>
#include <CS2/SDK/Math/Vector.hpp>
#include <CS2/SDK/Math/QAngle.hpp>

// Forward declarations
class C_BaseEntity;
class C_CSWeaponBase;

class CGrenadePrediction
{
public:
    void OnCreateMove(C_UserCmd* cmd); // Se você tiver acesso ao CreateMove, chame aqui
    void Render(); 

private:
    struct PathNode
    {
        Vector3 m_pos;
        bool m_grounded;
    };

    void Simulate(QAngle& angles, C_BaseEntity* pLocal, C_CSWeaponBase* pWeapon);
    void Setup(Vector3& src, Vector3& vecThrow, QAngle& viewangles, C_BaseEntity* pLocal, C_CSWeaponBase* pWeapon);
    size_t Step(Vector3& src, Vector3& velocity, float tick_interval, float interval);
    void ResolveFlyCollisionCustom(class CGameTrace& trace, Vector3& velocity, float interval); // PhysicsClipVelocity
    int PhysicsClipVelocity(const Vector3& in, const Vector3& normal, Vector3& out, float overbounce);

    std::vector<PathNode> m_path;
    int m_type = 0; // 0: HE/Flash, 1: Smoke, 2: Molotov
    int m_act = 0;  // Throw strength
};

inline CGrenadePrediction* GetGrenadePrediction() 
{
    static CGrenadePrediction instance;
    return &instance;
}
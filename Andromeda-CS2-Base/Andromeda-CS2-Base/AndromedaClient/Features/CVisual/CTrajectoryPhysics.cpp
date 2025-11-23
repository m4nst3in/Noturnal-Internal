#include "CTrajectoryPhysics.hpp"

#include <CS2/SDK/SDK.hpp>
#include <CS2/SDK/Update/GameTrace.hpp>
#include <CS2/SDK/FunctionListSDK.hpp>
#include <GameClient/CL_Players.hpp>
#include <GameClient/CL_Weapons.hpp>
#include <cmath>

// Configurações de Física da Source 2
#define TICK_INTERVAL           0.015625f 
#define TIME_TO_TICKS( dt )     ( (int)( 0.5f + (float)(dt) / TICK_INTERVAL ) )

// Matemática Auxiliar para Reflexão (PhysicsClipVelocity da Source Engine)
int CTrajectoryPhysics::PhysicsClipVelocity(const Vector3& in, const Vector3& normal, Vector3& out, float overbounce)
{
    float backoff = in.Dot(normal) * overbounce;
    for (int i = 0; i < 3; i++)
    {
        float change = normal[i] * backoff;
        out[i] = in[i] - change;
        if (out[i] > -0.1f && out[i] < 0.1f) out[i] = 0.0f; // Fix precisão
    }
    
    float adjust = out.Dot(normal);
    if (adjust < 0.0f) out -= (normal * adjust);
    return 0;
}

void CTrajectoryPhysics::SetupPhysics(C_CSWeaponBase* pWeapon, float& flSpeed, float& flGravity, float& flElasticity, float& flFriction, float& flDrag)
{
    // Valores padrão
    flSpeed = 1000.0f;      // HE/Flash
    flGravity = 800.0f;     // Gravidade padrão sv_gravity
    flElasticity = 0.45f;   // Quique
    flFriction = 0.2f;      // Atrito no chão
    flDrag = 0.0f;          // Arrasto do ar (Smokes novas são pesadas)

    if (!pWeapon) return;

    // Tente pegar o Index ou Nome da arma. 
    // Como não tenho sua classe C_CSWeaponBase completa, vou assumir pelo ID ou Nome
    // Adapte essa parte para como você pega o ID da arma (m_iItemDefinitionIndex)
    
    int nIndex = pWeapon->m_AttributeManager().m_Item().m_iItemDefinitionIndex();
    
    /* IDs (Exemplo):
       WEAPON_FLASHBANG = 43
       WEAPON_HEGRENADE = 44
       WEAPON_SMOKEGRENADE = 45
       WEAPON_MOLOTOV = 46
       WEAPON_DECOY = 47
       WEAPON_INCGRENADE = 48
    */

    if (nIndex == 45) { // SMOKE
         // Smokes no CS2 são um pouco diferentes, mas a física base é similar
         flElasticity = 0.3f; // Quica menos
    }
    else if (nIndex == 46 || nIndex == 48) { // MOLOTOV / INCENDIARY
         flSpeed = 750.0f; // Mais lento
         flElasticity = 0.35f;
    }
    else if (nIndex == 47) { // DECOY
         flElasticity = 0.5f; // Quica muito
    }
}

std::vector<GrenadePathNode> CTrajectoryPhysics::Simulate(C_CSPlayerPawn* pLocal, C_CSWeaponBase* pWeapon, bool bAttack1, bool bAttack2)
{
    std::vector<GrenadePathNode> path;
    if (!pLocal || !pWeapon) return path;

    // 1. Definir Força do Arremesso (Lógica exata do CS)
    float flThrowStrength = 0.0f;
    if (bAttack1 && bAttack2) flThrowStrength = 0.5f; // Médio
    else if (bAttack2) flThrowStrength = 0.3333f;     // Curto (Direito)
    else if (bAttack1) flThrowStrength = 1.0f;        // Longo (Esquerdo)
    else return path; // Não está arremessando

    if (flThrowStrength == 0.0f) return path;

    // 2. Setup Inicial
    QAngle angEye = pLocal->m_angEyeAngles();
    
    // Ajuste de Pitch para arremessos diferentes
    float flPitch = angEye.x;
    if (flThrowStrength < 1.0f)
    {
        // Se for arremesso fraco, o jogo subtrai pitch (faz a granada subir mais "curta")
        float flSub = (90.0f - abs(flPitch)) * 0.1111f; // Aproximação CS2
        if (flPitch < 0.0f) flPitch -= flSub;
        else flPitch += flSub;
    }
    
    // Recalcular vetores com o Pitch ajustado
    QAngle angThrow = angEye;
    angThrow.x = flPitch;
    
    Vector3 vForward, vRight, vUp;
    Math::AngleVectors(angThrow, vForward, vRight, vUp);

    // 3. Posição de Saída (Spawn da Granada)
    // No CS2, sai ligeiramente da direita da cabeça (menos que no CS:GO)
    Vector3 vStart = pLocal->m_vOldOrigin() + pLocal->m_vecViewOffset();
    
    // Pequeno offset para não sair de dentro da cara (Valores empíricos CS2)
    vStart += (vForward * 12.0f) + (vRight * -2.0f) + (vUp * -2.0f);

    // 4. Parâmetros Físicos
    float speed, gravity, elasticity, friction, drag;
    SetupPhysics(pWeapon, speed, gravity, elasticity, friction, drag);

    // Calcular Velocidade Inicial
    // Velocidade base * Força
    float flVel = speed * 0.9f; // Fator de correção CS2 (geralmente não sai com 100% da speed do script)
    flVel *= flThrowStrength; 

    // O vetor de velocidade inicial
    Vector3 vVelocity = vForward * flVel;

    // *** CORREÇÃO CRUCIAL: INÉRCIA DO JOGADOR ***
    // Se o jogador estiver correndo ou pulando, a granada herda velocidade.
    Vector3 vPlayerVel = pLocal->m_vecAbsVelocity(); 
    vVelocity += vPlayerVel * 1.25f; // 1.25 é o fator mágico da engine Source

    // 5. Loop de Simulação
    Vector3 vPos = vStart;
    path.push_back({ ImVec2(0,0), vPos, false }); // Ponto inicial

    // Configuração do Trace
    CTraceFilter filter(0x1C1003, pLocal, 3, 15); // World + Props, Ignore Local
    Ray_t ray;
    CGameTrace tr;

    // Simular por ~2.5 segundos (suficiente para a maioria das granadas)
    int nTicks = TIME_TO_TICKS(3.0f); 
    float flInterval = TICK_INTERVAL;

    for (int i = 0; i < nTicks; ++i)
    {
        // Integrar Gravidade (Euler simples ou Verlet)
        // v = v0 + a * t
        if (gravity != 0.0f)
             vVelocity.z -= (gravity * 0.5f) * flInterval; // Metade da gravidade antes

        // Arrasto (Air Resistance)
        // vVelocity *= (1.0f - drag * flInterval); 

        Vector3 vNextPos = vPos + (vVelocity * flInterval);

        // Integrar Gravidade (Segunda metade - Velocity Verlet simplificado)
        if (gravity != 0.0f)
             vVelocity.z -= (gravity * 0.5f) * flInterval;

        // RAYCAST (TraceShape)
        // Usando a função que você forneceu no CL_Trace
        // Hull de granada ~ 2x2x2
        bool bHit = false;

        // Limpar trace anterior
        memset(&tr, 0, sizeof(CGameTrace));

        if (IGamePhysicsQuery_TraceShape(SDK::Pointers::CVPhys2World(), ray, vPos, vNextPos, &filter, &tr))
        {
            if (tr.DidHit())
            {
                bHit = true;
                
                // Mover até o ponto de impacto
                vPos = tr.vecPosition;

                // Verificar se parou/explodiu
                // Molotov explode ao tocar chão (Normal.z > 0.7)
                int nIdx = pWeapon->m_AttributeManager().m_Item().m_iItemDefinitionIndex();
                if ((nIdx == 46 || nIdx == 48) && tr.vecNormal.z > 0.7f) // Molotov + Chão
                {
                    // Marca ponto final e encerra
                    ImVec2 s;
                    if (Math::WorldToScreen(vPos, s))
                        path.push_back({ s, vPos, true });
                    break;
                }

                // Resolver Colisão (Rebote)
                Vector3 vNewVel;
                PhysicsClipVelocity(vVelocity, tr.vecNormal, vNewVel, 1.0f + elasticity);
                vVelocity = vNewVel;

                // Aplicar Atrito de superfície
                if (tr.vecNormal.z > 0.7f) // Se bateu no chão
                {
                    vVelocity *= (1.0f - friction);
                }

                // Se a velocidade for muito baixa, a granada para
                if (vVelocity.LengthSqr() < (20.0f * 20.0f)) // Stop Speed
                {
                    break;
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

        // Adicionar ponto para renderizar
        // Para otimizar, não adicione TODOS os ticks, adicione a cada 2 ou 3
        if (i % 2 == 0 || bHit)
        {
            ImVec2 s;
            if (Math::WorldToScreen(vPos, s))
            {
                path.push_back({ s, vPos, bHit });
            }
        }
    }

    return path;
}
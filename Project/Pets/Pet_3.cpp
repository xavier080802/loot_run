#include "Pet_3.h"
#include "../UI/WorldText.h"
#include "../DesignPatterns/PostOffice.h"
#include "../Helpers/Vec2Utils.h"
#include "../Actor/Player.h"
#include "../Actor/Actor.h"
#include "../Actor/Combat.h"
#include "../GameDB.h"

void Pet_3::Setup(Player& _player)
{
    player = &_player;
    weap = GameDB::GetEquipmentData(EquipmentCategory::Melee, 7);
    attackCooldown = std::stof(data.extra.at("attackCooldown"));
    attImgTime = std::stof(data.extra.at("attImgTime"));

    //Skill cooldown scales with rank
    data.skillCooldown /= (int)rank;

    attackAnimTimer = 0.f;
}

bool Pet_3::DoSkill(const Pets::SkillCastData& _data)
{
    if (!weap) {
        return false;
    }
    //Find clicked gameobject
    GameObject* go = GameObjectManager::GetInstance()->QueryOnMouse();
    Actor* other{ dynamic_cast<Actor*>(go) };
    AEVec2 mpos{ GetMouseWorldVec() };
    if (!other || other->GetGOType() != GO_TYPE::ENEMY) {
        PostOffice::GetInstance()->Send("WorldTextManager", new ShowWorldTextMsg{ "No target", mpos });
        target = nullptr;
        return false;
    }
    //Target acquired.
    target = other;
    PostOffice::GetInstance()->Send("WorldTextManager", new ShowWorldTextMsg{ "LOCKED", mpos, {40, 200, 50, 255} });
    SetPathRefreshTime(0.5f); //Chase more accurately
    return true;
}

void Pet_3::SkillUpdate(float dt)
{
    if (target && attackTimer > 0.f) {
        attackTimer -= dt;
    }

    //Change texture when attacking
    renderingData->SetActiveTexture(attackAnimTimer > 0.f ? 1 : 0);
    if (attackAnimTimer > 0.f) {
        attackAnimTimer -= dt;
    }
}

void Pet_3::DoMovement(double dt)
{
    if (!target) {
        SetPathRefreshTime(1.25f);
        Pet::DoMovement(dt);
        return;
    }
    //Chase down target instead
    UpdatePathfinding((float)dt); //Timers
    AEVec2 tpos{ target->GetPos() };
    float sqrDist{ AEVec2SquareDistance(&pos, &tpos) };
    if (sqrDist < weap->attackSize * weap->attackSize) {
        //Attack
        Attack();
    }
    else { //Move closer to target
        Pathfinder::RESULT res = DoPathFinding(*tilemap, pos, tpos);
        if (res != Pathfinder::RESULT::FAILED) {
            std::deque<AEVec2> const& path{ GetFoundPath() };
            //Follow the path created by the pathfinder
            targetPos = path.empty() ? pos : path.front();
            MoveToTarget(dt);
        }
    }

    //Check if target dead
    if (target->IsDead() || !target->IsEnabled()) {
        target = nullptr;
    }
}

void Pet_3::Attack()
{
    if (attackTimer > 0 || !player || !weap || !target) return;
    attackTimer = attackCooldown;
    attackAnimTimer = attImgTime;
    Combat::ExecuteAttack(player, pos, weap, target->GetPos());
}

#include "Pet_2.h"
#include "../Actor/Player.h"

void Pet_2::Setup(Player& player)
{
    player.SubToOnHit(this);
}

bool Pet_2::DoSkill(const Pets::SkillCastData& _data)
{
    if (!_data.player) return false;
    //Shield value calc
    float shield{};
    shield += data.multipliers[0].GetValFromActor(*_data.player) + data.multipliers[1].GetValFromActor(*_data.player);
    _data.player->AddShield(shield);
    //Cache threshold for later
    threshold = data.multipliers[2].GetValFromActor(*_data.player);
    return true;
}

void Pet_2::SubscriptionAlert(OnHitContent content)
{
    //No shield, leave.
    if (!content.shieldMitigated || !content.actor) return; 

    //Debuff attacker
    if (content.attacker) {
        content.attacker->ApplyStatusEffect(new StatEffects::StatusEffect{ data.extraEffects[0] }, content.actor);
    }

    //Revenge damage if shield value threshold reached
    if (content.actor->GetShieldVal() + content.shieldMitigated >= threshold) {
        content.actor->DealDamage(content.attacker,
            data.multipliers[3].GetValFromActor(*content.actor) + data.multipliers[4].GetValFromActor(*content.actor)
            ,data.dmgTypes[0]);
    }
}

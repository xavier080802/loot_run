#include "AEMath.h"
#include "StatusEffect.h"
#include "Actor.h"
#include "../UI/UIManager.h"
#include "../Helpers/CoordUtils.h"
#include "../Helpers/RenderUtils.h"
#include <json/json.h>
#include <sstream>
#include <iomanip>

StatEffects::StatusEffect::~StatusEffect()
{
	if (uiElement) {
		uiElement->SetEnabled(false);
	}
}

StatEffects::StatusEffect* StatEffects::StatusEffect::AddMod(Mod newMod)
{
	mods.push_back(newMod);
	return this;
}

StatEffects::StatusEffect* StatEffects::StatusEffect::AddMod(std::vector<Mod> _mods)
{
	for (Mod const& m : _mods) {
		AddMod(m);
	}
	return this;
}

void StatEffects::StatusEffect::OnApply(Actor* _owner, Actor* _caster)
{
	owner = _owner;
	caster = _caster;

	uiElement = &(UIManager::GetInstance()->FetchOrphanUI()).SetHoverCallback([this](bool) {uiHovered = true;});
}

void StatEffects::StatusEffect::Tick(double dt)
{
	durationTimer += static_cast<float>(dt);

	if ((durationTimer >= duration && !isPermanent) || !stacks) {
		OnEnd(stacks ? END_REASON::TIMED_OUT : END_REASON::STACKS_ZERO);
	}
}

void StatEffects::StatusEffect::OnEnd(END_REASON reason)
{
	(void)reason; //Unused param
	hasEnded = true;
}

void StatEffects::StatusEffect::UpdateUI(bool showTooltip)
{
	if (showTooltip && uiHovered) {
		DrawTooltip();
	}
	//Reset flag
	uiHovered = false;
}

void StatEffects::StatusEffect::DrawTooltip() const
{
	static RenderingManager* rm{ RenderingManager::GetInstance() };
	//Form text
	std::ostringstream ss{};
	ss << std::fixed;
	ss << name;
	if (maxStacks <= 1) {
		ss << "\n[Unstackable]";
	}
	else {
		ss << "\nStacks: " << stacks << " [Max " + std::to_string(maxStacks) + "]";
	}
	for (Mod const& m : mods) {
		ss << "\n" << (m.value > 0 ? "+" : "")
			<< std::setprecision(1) << m.value << (m.mathType == StatEffects::MATH_TYPE::MULTIPLICATIVE ? "% " : " ")
			<< StatTypeToString(m.stat);
	}
	if (!isPermanent) {
		ss << "\n" << std::setprecision(1) << (duration - durationTimer) << "s";
	}
	std::string txt{ss.str()};

	s32 mx{}, my{};
	AEInputGetCursorPosition(&mx, &my);
	AEVec2 mP = ScreenToWorld(AEVec2{ (float)mx, (float)my });

	f32 nw{}, nh{};
	GetAEMultilineTextSize(rm->GetFont(), txt, 0.35f, nw, nh, 0.015f);
	AEVec2 nOffset = GetTextAlignPosNorm(rm->GetFont(), txt, mP, 0.35f, TEXT_LOWER_LEFT);

	//Draw text box
	DrawAETextbox(rm->GetFont(), txt, AEVec2{ (float)mP.x, (float)mP.y },
		AEGfxGetWinMaxX() * 0.4f, 0.35f, 0.015f, {0,0,0,255}, TEXT_LOWER_LEFT, TEXTBOX_ORIGIN_POS::BOTTOM,
		TextboxBgCfg{ {0.03f, 0.03f}, Color{150,150,150,200}, 255, rm->GetMesh(MESH_SQUARE) });
}

void StatEffects::StatusEffect::OnReapply(int numStacks)
{
	if (stacks < maxStacks) {
		//Add stacks up to the cap.
		stacks += numStacks;
		if (stacks > maxStacks) stacks = maxStacks;
	}

	//Refresh timer
	durationTimer = 0.f;
}

float StatEffects::StatusEffect::GetFinalModVal(STAT_TYPE stat, float baseVal) const
{
	float change{};
	for (Mod m : mods) {
		if (m.stat != stat) continue;
		change += (m.mathType == FLAT ? m.value : baseVal * (m.value / 100.f));
	}
	return change * stacks;
}

void StatEffects::StatusEffect::ScaleMods(float scalar)
{
	for (Mod& m : mods) {
		m.value *= scalar;
	}
}

void StatEffects::StatusEffect::SetIcon(std::string const& path)
{
	icon = path;
}

void StatEffects::StatusEffect::SetName(std::string const& newName)
{
	name = newName;
}

StatEffects::StatusEffect StatEffects::StatusEffect::ParseFromJson(Json::Value const& v)
{
	//Parse eff type string by changing to upper case first
	EFF_TYPE et{NONE};
	std::string _typeStr{ v.get("type", "").asString() };
	std::transform(_typeStr.begin(), _typeStr.end(), _typeStr.begin(), [](char c) {return static_cast<char>(std::toupper(c));});
	if (_typeStr == "BUFF") et = BUFF;
	else if (_typeStr == "DEBUFF") et = DEBUFF;
	//Create copy to store
	StatusEffect out{ nullptr, v.get("duration", -1).asFloat(), v.get("maxStacks", 1).asUInt(), v.get("name","").asString(),
	v.get("startStacks",1U).asUInt(), et, v.get("icon", "auto").asString()};

	//Add mods
	if (v.findArray("mods")) {
		for (Json::Value const& m : v["mods"]) {
			//Each value should be a mod
			out.AddMod(StatEffects::Mod::ParseFromJSON(m));
		}
	}
	return out;
}

float StatEffects::Mod::GetValFromActor(Actor const& actor) const
{
	if (mathType == FLAT) return value;

	const ActorStats& stats{ actor.GetStats() };
	float out{};
	switch (stat)
	{
	case MAX_HP:
		out = stats.maxHP;
		break;
	case DEF:
		out = stats.defense;
		break;
	case ATT:
		out = stats.attack;
		break;
	case ATT_SPD:
		out = stats.attackSpeed;
		break;
	case MOVE_SPD:
		out = stats.moveSpeed;
		break;
	default:
		break;
	}

	return out * (value / 100.f);
}

StatEffects::Mod StatEffects::Mod::ParseFromJSON(Json::Value const& v)
{
	Mod out{};
	out.value = v.get("value", 0).asFloat();
	if (v.find("stat")) {
		if (v["stat"].isString()) {
			out.stat = ParseStatTypeFromStr(v.get("stat", "att").asCString());
		}
		else out.stat = static_cast<STAT_TYPE>(v.get("stat", STAT_TYPE::ATT).asInt());
	}

	if (v["mathType"].isString()) {
		std::string s{ v.get("mathType", "flat").asString() };
		std::transform(s.begin(), s.end(), s.begin(), [](char c) {return static_cast<char>(std::toupper(c));});
		if (s == "MULT" || s == "MULTIPLICATIVE" || s == "*" || s == "%") out.mathType = MATH_TYPE::MULTIPLICATIVE;
		else out.mathType = MATH_TYPE::FLAT;
	}
	else out.mathType = static_cast<StatEffects::MATH_TYPE>(v.get("mathType", StatEffects::MATH_TYPE::MULTIPLICATIVE).asInt());
	out.tag = v.get("tag", "").asString();
	return out;
}

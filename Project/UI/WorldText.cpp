#include "WorldText.h"
#include "../DesignPatterns/PostOffice.h"
#include "../Helpers/RenderUtils.h"
#include "../RenderingManager.h"

void WorldTextManager::Init() {
	PostOffice::GetInstance()->Register("WorldTextManager", this);
	texts.reserve(50);
	rm = RenderingManager::GetInstance();
}

bool WorldTextManager::Handle(Message* message) {
	ShowWorldTextMsg* msg{ dynamic_cast<ShowWorldTextMsg*>(message) };
	if (!msg) {
		delete message;
		return false;
	}
	//Add the new msg to list
	for (Text& t : texts) { //Finding existing inactive
		if (t.timer > 0) continue;
		t = Text{ *msg };
		delete message;
		return true;
	}
	//No inactive, push back new
	texts.push_back(Text{ *msg });
	delete message;
	return true;

}

void WorldTextManager::Update(double dt) {
	//Update all texts
	for (Text& t : texts) {
		if (t.timer <= 0.f) continue;
		t.timer -= (float)dt;
		t.col.a = 255 * t.timer;
	}
}

void WorldTextManager::Draw() {
	for (Text const& t : texts) {
		if (t.timer <= 0.f) continue;
		DrawAEText(rm->GetFont(), t.text.c_str(), t.pos, t.size / rm->GetFontSize(), t.col, TEXT_MIDDLE, false);
	}
}

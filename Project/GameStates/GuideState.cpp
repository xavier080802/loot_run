#include "GuideState.h"
#include <json/json.h>
#include "../Helpers/Vec2Utils.h"
#include "../Helpers/MatrixUtils.h"
#include "../UI/UIElement.h"
#include "../UI/UIManager.h"
#include "../InputManager.h"
#include "../Music.h"
#include <fstream>
#include <cstdio>
#include <iostream>

extern BGMManager bgm;

void GuideState::LoadState() {
	LoadUIJSON();
	rm = RenderingManager::GetInstance();
	font = rm->GetFont();
}

void GuideState::InitState() {
	currPage = 0;
	exitHovered = prevHovered = nextHovered = false;

	//Disable input manager, this state's priority should be very high
	//Prevents inputs on the previous state from still triggering while this state is here.
	prevInputPrio = InputManager::GetInstance()->GetMinPrio();
	InputManager::GetInstance()->SetMinPrio(inputPrio);
}

void GuideState::Update(double /*dt*/) {
	if (AEInputCheckTriggered(AEVK_ESCAPE)) {
		GameStateManager::GetInstance()->ReturnToPrevState(false);
	}

	//Refresh - hot reload json data
	if (AEInputCheckTriggered(AEVK_F5)) {
		pages.clear();
		LoadUIJSON();
		if (currPage >= pages.size()) {
			currPage = static_cast<int>(pages.size()) - 1;
		}
	}
}

void GuideState::Draw() {
	if (pages.empty()) { //Fallback display if there are no pages
		DrawAEText(font, "Oops! There's no content here... JSON exploded? :<", {}, 1, Color{ 255,100,100,255 }, TEXT_ORIGIN_POS::TEXT_MIDDLE);
		return;
	}

	// ===================== Pagination text =====================
	char buf[20];
	sprintf_s(buf, pageIndicator.format.c_str(), currPage + 1, pages.size());
	DrawAEText(font, buf, pageIndicator.pos, pageIndicator.fontSize, pageIndicator.color, pageIndicator.origin);

	//===================== Draw the current page =====================
	Page const& p{ pages.at(currPage) };
	//Title
	DrawAEText(font, p.titleText.c_str(), titlePos, titleFontSz, p.titleCol, TEXT_MIDDLE);
	for (Content const& c : p.content) {
		if (!c.hardcodeInstead) {
			//Content
			DrawAETextbox(font, c.text, c.contentPos, c.contentWidth, c.contentFontSize, c.contentLineSpace, c.contentCol, c.contentTxtOrigin, TEXTBOX_ORIGIN_POS::TOP);
			//Images
			for (Img const& img : c.imgPaths) {
				DrawMesh(GetTransformMtx(img.pos, 0, img.size), rm->GetMesh(MESH_SQUARE), rm->LoadTexture(img.path), 255);
			}
		}
		else {
			// First, Check if the current page is the correct page
			// Then, put your code to render

		}
	}

	//===================== Buttons =====================
	if (prevBtn) { //Pagination previous
		DrawTintedMesh(GetTransformMtx(prevBtn->GetPos(), 0, prevBtn->GetSize()),
			rm->GetMesh(MESH_SQUARE), nullptr, prevHovered ? btnHoverCol : btnCol, 255);
		DrawAEText(font, prevText.c_str(), prevBtn->GetPos(), btnFontSz, btnTextCol, TEXT_MIDDLE);
	}
	if (nextBtn) { //Pagination next
		DrawTintedMesh(GetTransformMtx(nextBtn->GetPos(), 0, nextBtn->GetSize()),
			rm->GetMesh(MESH_SQUARE), nullptr, nextHovered ? btnHoverCol : btnCol, 255);
		DrawAEText(font, nextText.c_str(), nextBtn->GetPos(), btnFontSz, btnTextCol, TEXT_MIDDLE);
	}
	if (exitBtn) { //Leave state
		DrawTintedMesh(GetTransformMtx(exitBtn->GetPos(), 0, exitBtn->GetSize()),
			rm->GetMesh(MESH_SQUARE), nullptr, exitHovered ? btnHoverCol : btnCol, 255);
		DrawAEText(font, exitText.c_str(), exitBtn->GetPos(), btnFontSz, btnTextCol, TEXT_MIDDLE);
	}

	//Reset hover state for btns at the end of the frame
	exitHovered = prevHovered = nextHovered = false;
}

void GuideState::ExitState() {
	//Set input manager value back to original
	InputManager::GetInstance()->SetMinPrio(prevInputPrio);
}

void GuideState::UnloadState() {
	pages.clear();
}

void GuideState::LoadUIJSON() {
	std::ifstream ifs{ "Assets/Data/ui.json", std::ios_base::binary };
	if (!ifs.is_open()) return;

	Json::Value root;
	Json::CharReaderBuilder builder;
	std::string errs;

	if (!Json::parseFromStream(builder, ifs, &root, &errs) || !root.isMember("guide")) {
		std::cout << "Guide: parse failed: " << errs << "\n";
		return;
	}
	root = root["guide"];

	if (root.isMember("btnCol") && root["btnCol"].size() == 4) {
		btnCol = Color{ root["btnCol"][0].asFloat(), root["btnCol"][1].asFloat(),
			root["btnCol"][2].asFloat() ,root["btnCol"][3].asFloat() };
	}
	if (root.isMember("btnHoverCol") && root["btnHoverCol"].size() == 4) {
		btnHoverCol = Color{ root["btnHoverCol"][0].asFloat(), root["btnHoverCol"][1].asFloat(),
			root["btnHoverCol"][2].asFloat() ,root["btnHoverCol"][3].asFloat() };
	}
	if (root.isMember("btnTextCol") && root["btnTextCol"].size() == 4) {
		btnTextCol = Color{ root["btnTextCol"][0].asFloat(), root["btnTextCol"][1].asFloat(),
			root["btnTextCol"][2].asFloat() ,root["btnTextCol"][3].asFloat() };
	}
	btnFontSz = root.get("btnFontSize", 1.f).asFloat();
	titleFontSz = root.get("titleFontSize", 1.f).asFloat();
	if (root.isMember("titlePos") && root["titlePos"].size() == 2) {
		titlePos = AEVec2{ root["titlePos"][0].asFloat(), root["titlePos"][1].asFloat() };
	}

	if (root.isMember("prevBtn")) {
		prevText = root["prevBtn"].get("text", "Previous").asString();
		//Setup the btn from UIElement
		prevBtn = prevBtn ? &prevBtn->ReInit(root["prevBtn"]) : new UIElement{root["prevBtn"]};
		prevBtn->SetHoverCallback([this](bool) {prevHovered = true;})
			.SetClickCallback([this]() {
				//Go back a page
				--currPage;
				if (currPage == -1) {
					currPage = static_cast<int>(pages.size()) - 1;
				}
				bgm.PlayUIClick();
			}).SetPriority(inputPrio);
	}
	if (root.isMember("nextBtn")) {
		nextText = root["nextBtn"].get("text", "Next").asString();
		nextBtn = nextBtn ? &nextBtn->ReInit(root["nextBtn"]) : new UIElement{ root["nextBtn"] };
		nextBtn->SetHoverCallback([this](bool) {nextHovered = true;})
			.SetClickCallback([this]() {
				//Go forward a page
				++currPage;
				if (currPage > pages.size() -1) {
					currPage = 0;
				}
				bgm.PlayUIClick();
			}).SetPriority(inputPrio);
	}
	if (root.isMember("exitBtn")) {
		exitText = root["exitBtn"].get("text", "Exit").asString();
		exitBtn = exitBtn ? &exitBtn->ReInit(root["exitBtn"]) : new UIElement{ root["exitBtn"] };
		exitBtn->SetClickCallback([]() {
			//Leave state. Do not call Init as it is assumed that the prev state is
			//in a paused state
			GameStateManager::GetInstance()->ReturnToPrevState(false);
			bgm.PlayUIClick();
		})
		.SetHoverCallback([this](bool) {exitHovered = true;}).SetPriority(inputPrio);
	}

	if (root.isMember("pageIndicator")) {
		Json::Value& pi{ root["pageIndicator"] };
		if (pi.isMember("pos") && pi["pos"].size() == 2) {
			pageIndicator.pos = AEVec2{ pi["pos"][0].asFloat(), pi["pos"][1].asFloat() };
		}
		if (pi.isMember("color") && pi["color"].size() == 4) {
			pageIndicator.color = Color{ pi["color"][0].asFloat(), pi["color"][1].asFloat(),
				pi["color"][2].asFloat() ,pi["color"][3].asFloat() };
		}
		pageIndicator.fontSize = pi.get("fontSize", 1.f).asFloat();
		pageIndicator.format = pi.get("format", "%i of $i").asString();
		pageIndicator.origin = ParseTextAlignment(pi.get("origin", "").asString());
	}

	//Pages (array)
	if (root.isMember("pages") && root["pages"].isArray()) {
		Json::Value& pgs{ root["pages"] };
		pages.reserve(pgs.size());
		for (Json::Value& v : pgs) {
			Page p{};
			p.titleText = v.get("titleText", "").asString();

			if (v.isMember("content") && v["content"].isArray()) {
				for (Json::Value& c : v["content"]) {
					Content ct{};
					ct.hardcodeInstead = c.get("hardcodeInstead", false).asBool();
					if (c.isMember("contentCol") && c["contentCol"].size() == 4) {
						ct.contentCol = Color{ c["contentCol"][0].asFloat(), c["contentCol"][1].asFloat(),
							c["contentCol"][2].asFloat() ,c["contentCol"][3].asFloat() };
					}
					ct.text = c.get("text", "").asString();
					ct.contentTxtOrigin = ParseTextAlignment(c.get("contentTxtOrigin", "").asString());
					if (c.isMember("imgPaths") && c["imgPaths"].isArray()) {
						for (Json::Value& i : c["imgPaths"]) {
							Img img{};
							img.path = i.get("path", "").asString();
							if (i.isMember("size") && i["size"].size() == 2) {
								img.size = AEVec2{ i["size"][0].asFloat(), i["size"][1].asFloat() };
							}
							if (i.isMember("pos") && i["pos"].size() == 2) {
								img.pos = AEVec2{ i["pos"][0].asFloat(), i["pos"][1].asFloat() };
							}
							ct.imgPaths.push_back(img);
						}
					}
					if (c.isMember("contentPos") && c["contentPos"].size() == 2) {
						ct.contentPos = AEVec2{ c["contentPos"][0].asFloat(), c["contentPos"][1].asFloat() };
					}
					ct.contentWidth = c.get("contentWidth", 1000.f).asFloat();
					ct.contentFontSize = c.get("contentFontSize", 0.5f).asFloat();
					ct.contentLineSpace = c.get("contentLineSpace", 0.015f).asFloat();

					p.content.push_back(ct);
				}
			}
			
			if (v.isMember("titleCol") && v["titleCol"].size() == 4) {
				p.titleCol = Color{ v["titleCol"][0].asFloat(), v["titleCol"][1].asFloat(),
					v["titleCol"][2].asFloat() ,v["titleCol"][3].asFloat() };
			}

			pages.push_back(p);
		}
	}
	else {
		std::cout << "Guide: pages must be an array\n";
		return;
	}
}
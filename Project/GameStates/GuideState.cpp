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
	prevInputPrio = InputManager::GetInstance()->GetMinPrio();
	InputManager::GetInstance()->SetMinPrio(inputPrio);
}

void GuideState::Update(double dt) {
	if (AEInputCheckTriggered(AEVK_ESCAPE)) {
		GameStateManager::GetInstance()->ReturnToPrevState(false);
	}

	//Refresh
	if (AEInputCheckTriggered(AEVK_F5)) {
		pages.clear();
		LoadUIJSON();
	}
}

void GuideState::Draw() {
	if (pages.empty()) {
		DrawAEText(font, "Oops! There's no content here... JSON exploded? :<", {}, 1, Color{ 255,100,100,255 }, TextOriginPos::TEXT_MIDDLE);
		return;
	}

	// ===================== Page indicator =====================
	char buf[20];
	sprintf_s(buf, pageIndicator.format.c_str(), currPage + 1, pages.size());
	DrawAEText(font, buf, pageIndicator.pos, pageIndicator.fontSize, pageIndicator.color, pageIndicator.origin);

	//===================== Draw the current page =====================
	Page const& p{ pages.at(currPage) };
	if (!p.hardcodeInstead) {
		//Title
		DrawAEText(font, p.titleText.c_str(), titlePos, titleFontSz, p.titleCol, TEXT_MIDDLE);
		//Content
		DrawAETextbox(font, p.contentText, p.contentPos, p.contentWidth, p.contentFontSize, p.contentLineSpace, p.contentCol, p.contentTxtOrigin, TextboxOriginPos::TOP);
		//Images
		for (Img const& img : p.imgPaths) {
			DrawMesh(GetTransformMtx(img.pos, 0, img.size), rm->GetMesh(MESH_SQUARE), rm->LoadTexture(img.path), 255);
		}
	}
	else {
		// First, Check if the current page is the correct page
		// Then, put your code to render

	}

	//===================== Buttons =====================
	if (prevBtn) {
		DrawTintedMesh(GetTransformMtx(prevBtn->GetPos(), 0, prevBtn->GetSize()),
			rm->GetMesh(MESH_SQUARE), nullptr, prevHovered ? btnHoverCol : btnCol, 255);
		DrawAEText(font, prevText.c_str(), prevBtn->GetPos(), btnFontSz, btnTextCol, TEXT_MIDDLE);
	}
	if (nextBtn) {
		DrawTintedMesh(GetTransformMtx(nextBtn->GetPos(), 0, nextBtn->GetSize()),
			rm->GetMesh(MESH_SQUARE), nullptr, nextHovered ? btnHoverCol : btnCol, 255);
		DrawAEText(font, nextText.c_str(), nextBtn->GetPos(), btnFontSz, btnTextCol, TEXT_MIDDLE);
	}
	if (exitBtn) {
		DrawTintedMesh(GetTransformMtx(exitBtn->GetPos(), 0, exitBtn->GetSize()),
			rm->GetMesh(MESH_SQUARE), nullptr, exitHovered ? btnHoverCol : btnCol, 255);
		DrawAEText(font, exitText.c_str(), exitBtn->GetPos(), btnFontSz, btnTextCol, TEXT_MIDDLE);
	}

	//Reset hover state for btns
	exitHovered = prevHovered = nextHovered = false;
}

void GuideState::ExitState() {
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
		prevBtn = prevBtn ? &prevBtn->ReInit(root["prevBtn"]) : new UIElement{root["prevBtn"]};
		prevBtn->SetHoverCallback([this](bool) {prevHovered = true;})
			.SetClickCallback([this]() {
				--currPage;
				if (currPage == -1) {
					currPage = pages.size()-1;
				}
				bgm.PlayUIClick();
			}).SetPriority(inputPrio);
	}
	if (root.isMember("nextBtn")) {
		nextText = root["nextBtn"].get("text", "Next").asString();
		nextBtn = nextBtn ? &nextBtn->ReInit(root["nextBtn"]) : new UIElement{ root["nextBtn"] };
		nextBtn->SetHoverCallback([this](bool) {nextHovered = true;})
			.SetClickCallback([this]() {
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
			p.hardcodeInstead = v.get("hardcodeInstead", false).asBool();
			p.titleText = v.get("titleText", "").asString();
			p.contentText = v.get("contentText", "").asString();
			p.contentTxtOrigin = ParseTextAlignment(v.get("contentTxtOrigin", "").asString());
			
			if (v.isMember("titleCol") && v["titleCol"].size() == 4) {
				p.titleCol = Color{ v["titleCol"][0].asFloat(), v["titleCol"][1].asFloat(),
					v["titleCol"][2].asFloat() ,v["titleCol"][3].asFloat() };
			}
			if (v.isMember("contentCol") && v["contentCol"].size() == 4) {
				p.contentCol = Color{ v["contentCol"][0].asFloat(), v["contentCol"][1].asFloat(),
					v["contentCol"][2].asFloat() ,v["contentCol"][3].asFloat() };
			}

			if (v.isMember("imgPaths") && v["imgPaths"].isArray()) {
				for (Json::Value& i : v["imgPaths"]) {
					Img img{};
					img.path = i.get("path", "").asString();
					if (i.isMember("size") && i["size"].size() == 2) {
						img.size = AEVec2{ i["size"][0].asFloat(), i["size"][1].asFloat() };
					}
					if (i.isMember("pos") && i["pos"].size() == 2) {
						img.pos = AEVec2{ i["pos"][0].asFloat(), i["pos"][1].asFloat() };
					}
					p.imgPaths.push_back(img);
				}
			}

			if (v.isMember("contentPos") && v["contentPos"].size() == 2) {
				p.contentPos = AEVec2{ v["contentPos"][0].asFloat(), v["contentPos"][1].asFloat() };
			}
			p.contentWidth = v.get("contentWidth", 0.5f).asFloat();
			p.contentFontSize = v.get("contentFontSize", 0.5f).asFloat();
			p.contentLineSpace = v.get("contentLineSpace", 0.15f).asFloat();

			pages.push_back(p);
		}
	}
	else {
		std::cout << "Guide: pages must be an array\n";
		return;
	}
}
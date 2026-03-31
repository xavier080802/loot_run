#ifndef _GUIDE_STATE_H
#define _GUIDE_STATE_H
#include "../GameStateManager.h"
#include "AEEngine.h"
#include "../RenderingManager.h"
#include "../Helpers/ColorUtils.h"
#include "../Helpers/RenderUtils.h"
#include <vector>

class UIElement;

class GuideState : public State
{
public:
	//Load asset resources. Is called when added to the manager.
	void LoadState();
	//Initialize state to starting values / Clean slate
	void InitState();
	//Update state logic
	void Update(double dt);
	//Draw state
	void Draw();
	//Exits state without freeing assets
	void ExitState();
	//Frees resources from memory (if any). Signifies the end of this state. Called when manager is destroyed.
	void UnloadState();
private:
	void LoadUIJSON();

	int prevInputPrio{};
	const int inputPrio{ 50 };
	RenderingManager* rm{};
	s8 font{};
	int currPage{};
	Color btnCol, btnHoverCol, btnTextCol;
	bool exitHovered, prevHovered, nextHovered;

	AEVec2 titlePos;
	float titleFontSz;

	struct Img {
		std::string path;
		AEVec2 size, pos;
	};

	struct Page {
		std::string titleText, contentText;
		TextOriginPos contentTxtOrigin;
		Color titleCol{ 255,255,255,255 }, contentCol{255,255,255,255};
		//Filepath + size
		std::vector<Img> imgPaths;
		AEVec2 contentPos;
		float contentFontSize, contentWidth, contentLineSpace;
	};
	std::vector<Page> pages;

	struct {
		AEVec2 pos;
		float fontSize;
		Color color;
		std::string format;
		TextOriginPos origin;
	} pageIndicator;

	std::string nextText, prevText, exitText;
	float btnFontSz;
	UIElement* nextBtn, * prevBtn, *exitBtn;
};

#endif // !_GUIDE_STATE_H

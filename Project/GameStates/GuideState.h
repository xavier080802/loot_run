#ifndef _GUIDE_STATE_H
#define _GUIDE_STATE_H
#include "GameStateManager.h"
#include "AEEngine.h"
#include "../Rendering/RenderingManager.h"
#include "../Helpers/ColorUtils.h"
#include "../Helpers/RenderUtils.h"
#include <vector>

/*	Guide State
	
	Purpose: Show content to guide the player through some stuff.
			Accessed from the main menu and pause menu

	NOTE: When entering this state from GameState, DO NOT exit the current state in the parameters,
	and set disableGOs to false, otherwise GameState will explode.

	IMPORTANT: This state will not call Init of the previous state, so when going to this state, do not exit the current one.

	Extra: Press F5 to re-load from json. 
		Remember that in Debug mode, the json file that is accessed is in the bin folder.
		Copy-paste your changes from the bin json to the Assets/Data json. If the project builds again before this, GG

	Editing the JSON:
	 - ui.json
	 - object named "guide"

	 Contains several variables for generic settings like the button colours and size.
	 pageIndicator - Text that shows what page the user is on.
	 prev/next btns - Buttons to change page (-- and ++)
	 exitBtn - Button to exit the state. Calls ReturnToPrevState.

	 pages: Array of Page objects. Adhere to the format written in the comments of the json.
	  - Each page has a title. The main content is referred to as "content".
	  - A page can have multiple content sections
	 
	 content: Array of Content objects. Adhere to the format written in the comments of the json.
	  - Text, images, or both. In the json, variables named "content____" are referring to the text content.
	  - Can have multiple images through imgPaths array
	  - For each entry to the array, the position of the text and images are to be written in absolute values.
*/

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
	//Load UI config from JSON.
	//Also allocates the UIElements for the buttons.
	void LoadUIJSON();

	int prevInputPrio{};
	const int inputPrio{ 50 };
	RenderingManager* rm{};
	s8 font{};
	int currPage{};
	Color btnCol{}, btnHoverCol{}, btnTextCol{};
	bool exitHovered{}, prevHovered{}, nextHovered{};

	AEVec2 titlePos{};
	float titleFontSz{};

	struct Img {
		std::string path{};
		AEVec2 size{}, pos{};
	};

	struct Content {
		std::string text{};
		TEXT_ORIGIN_POS contentTxtOrigin{};
		Color contentCol{ 255,255,255,255 };
		//Filepath + size
		std::vector<Img> imgPaths{};
		AEVec2 contentPos{};
		float contentFontSize{}, contentWidth{}, contentLineSpace{};
		//Hardcode content
		bool hardcodeInstead{};
	};

	struct Page {
		std::string titleText{};
		Color titleCol{ 255,255,255,255 };

		std::vector<Content> content{};
	};
	std::vector<Page> pages{};

	struct {
		AEVec2 pos{};
		float fontSize{};
		Color color{};
		std::string format{};
		TEXT_ORIGIN_POS origin{};
	} pageIndicator;

	std::string nextText{}, prevText{}, exitText{};
	float btnFontSz{};
	UIElement* nextBtn{}, * prevBtn{}, * exitBtn{};
};

#endif // !_GUIDE_STATE_H

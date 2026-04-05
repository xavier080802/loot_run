#ifndef WORLDTEXT_H_
#define WORLDTEXT_H_

#include "../DesignPatterns/singleton.h"
#include "../DesignPatterns/PostBox.h"
#include "AEEngine.h"
#include "../Helpers/ColorUtils.h"
#include <vector>

class RenderingManager;

class WorldTextManager : public Singleton<WorldTextManager>, public PostBox
{
	friend Singleton<WorldTextManager>;
public:
	void Init();

	bool Handle(Message* message);

	void Update(double dt);
	void Draw();

private:
	struct Text {
		std::string text;
		AEVec2 pos;
		Color col;
		float size;
		float timer;

		Text(ShowWorldTextMsg const& msg) 
			: text{ msg.text }, pos{ msg.pos }, col{ msg.col }, size{ msg.size }, timer{ msg.duration } {}
	};
	std::vector<Text> texts{};

	RenderingManager* rm{};

};

#endif // WORLDTEXT_H_


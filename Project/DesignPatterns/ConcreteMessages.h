#ifndef _CONCRETE_MSG_H_
#define _CONCRETE_MSG_H_
#include "message.h"
#include "AEEngine.h"
#include "../Helpers/ColorUtils.h"
#include <string>

/* File to store all the Messages derived from the Message class

Example:

struct MyMessage : public Message{
	MyMessage(std::string _msg, Entity* _ent): msg(_msg), myBestie(_ent) {}	//constructor
	~MyMessage(){}	//destructor

	//Message contents
	std::string msg;
	Entity* myBestie;
}

//Send through post office:
PostOffice::GetInstance().Send("TargetAddress", new MyMessage("hi there", otherEnt));

*/

//Message about pet skill
struct PetSkillMsg : public Message {
	enum PetMessageType {
		CAST_SKILL,
		SKILL_READY,
	};
	PetSkillMsg(PetMessageType _type) : type(_type) {}
	~PetSkillMsg() {};

	PetMessageType type{};
};

//Message about showing text in the world
struct ShowWorldTextMsg : public Message {
	//size: Absolute size of the text
	ShowWorldTextMsg(std::string const& _text, AEVec2 _pos, Color _col = { 247, 231, 0, 255 }, float _dur = 1.5f, float _size = 20)
		: text{ _text }, pos{ _pos }, duration {_dur}, col{ _col }, size{ _size } {
	};
	~ShowWorldTextMsg() {}

	std::string const& text;
	AEVec2 pos;
	float duration;
	Color col;
	float size;
};

#endif // !_CONCRETE_MSG_H_

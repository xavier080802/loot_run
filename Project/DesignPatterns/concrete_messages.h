#ifndef _CONCRETE_MSG_H_
#define _CONCRETE_MSG_H_
#include "message.h"

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
PostOffice::GetInstance().Send("targetAddress", new MyMessage("hi there", otherEnt));

*/


#endif // !_CONCRETE_MSG_H_

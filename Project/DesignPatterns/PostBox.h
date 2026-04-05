#ifndef POSTBOX_H_
#define POSTBOX_H_

#include "ConcreteMessages.h"

//To inherit as class. Override bool Handle(Message* message)
//Required to receive messages from the Post Office
class PostBox {
public:
	virtual ~PostBox() {}

	//ALWAYS call delete message at the end of the function / after reading message.
	//Use dynamic_cast<MsgClass>(message) to cast the message to another type. Check null afterwards.
	//Return whether the message was read or not(invalid/not implemented)
	virtual bool Handle(Message* message) = 0;
};

#endif // POSTBOX_H_


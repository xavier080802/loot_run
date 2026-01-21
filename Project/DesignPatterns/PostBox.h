#ifndef _POST_BOX_H_
#define _POST_BOX_H_
#include "ConcreteMessages.h"

//To inherit as class. Override bool Handle(Message* message)
//Required to receive messages from the Post Office
class PostBox {
public:
	virtual ~PostBox() {}

	//After reading the message, it should be deleted.
	virtual bool Handle(Message* message) = 0;
};
#endif // !_POST_BOX_H_

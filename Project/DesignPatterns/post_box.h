#ifndef _POST_BOX_H_
#define _POST_BOX_H_
#include "message.h"

//To inherit as class.
class PostBox {
public:
	virtual ~PostBox() {}

	//After reading the message, it must be deleted.
	virtual bool Handle(Message* message) = 0;
};
#endif // !_POST_BOX_H_

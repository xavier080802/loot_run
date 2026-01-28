#ifndef POST_OFFICE_H
#define POST_OFFICE_H

#include "singleton.h"

#include <string>
#include <map>
#include "PostBox.h"
#include "Message.h"

/// <summary>
/// Singleton object that passes Message objects between a sender and
/// a target PostBox.
/// 
/// Enables some decoupling between files.
/// </summary>
class PostOffice : public Singleton<PostOffice>
{
	friend Singleton<PostOffice>;
public:
	//Registers a postbox with a unique address
	void Register(const std::string &address, PostBox* object);
	//Send a message to 1 receiver via address
	bool Send(const std::string &address, Message *message);
	//Send a message directly to the postbox
	bool Send(PostBox* receiver, Message* message);
	
private:
	PostOffice();
	~PostOffice();
	std::map<std::string, PostBox*> addressBook;
};

#endif

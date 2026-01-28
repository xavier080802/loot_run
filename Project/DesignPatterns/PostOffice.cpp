#include "PostOffice.h"

void PostOffice::Register(const std::string & address, PostBox * object)
{
	if (!object)
		return;
	if (addressBook.find(address) != addressBook.end())
		return;
	addressBook.insert(std::pair<std::string, PostBox*>(address, object));
}

bool PostOffice::Send(const std::string & address, Message * message)
{
	if (!message)
		return false;
	std::map<std::string, PostBox*>::iterator it = addressBook.find(address);
	if (addressBook.find(address) == addressBook.end())
	{
		delete message;
		return false;
	}
	PostBox *object = (PostBox*)it->second;
	return object->Handle(message);
}

bool PostOffice::Send(PostBox* receiver, Message* message)
{
	if (!message) return false;
	if (!receiver) {
		delete message;
		return false;
	}
	return receiver->Handle(message);
}

int PostOffice::Broadcast(Message& message)
{
	int count{};
	for (auto& pair : addressBook) {
		Message* msg = new Message(message);
		count += Send(pair.second, msg) ? 1 : 0;
	}
	return count;
}

PostOffice::PostOffice()
{
}

PostOffice::~PostOffice()
{
	addressBook.clear();
}

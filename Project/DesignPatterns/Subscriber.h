#ifndef _SUBSCRIBER_H_
#define _SUBSCRIBER_H_
/*
Unlike Post Office, content creators (CC, the ones alerting subscribers) do not
need to know their subscribers by name/address when sending stuff.
They simply know they have subscribers, and when there is content to send,
They will "upload" and subscribers will receive.

It is typically the Subscriber's job to subscribe to the CC,
but either way works (just a bit confusing)

---------------------------------------------------------------------

Subscriber class usage:

1. Subscriber classes inherit Subscriber<T>,
	where T is the type of the payload (Eg. struct) -> The content delivered by the CC.

2. Subscriber overrides pure-virtual SubscriptionAlert with type T.
	Syntax: void SubscriptionAlert(MyType) override = 0;
	This function is called by the CC.

3. Another class extends the DerivedSubscriber class
	and defines SubscriptionAlert to do whatever with the content.

---------------------------------------------------------------------

Content Creator implementation:

Have an array/vector/list/etc of type [DerivedSubscriber].

When sending content, iterate through and call each subscriber's SubscriptionAlert and pass in the content.

Recommended to implement a Subscribe and Unsubscribe function in the CC class.

*/
template<typename T>
struct Subscriber {
	virtual void SubscriptionAlert(T content) = 0;
};

#endif // !_SUBSCRIBER_H_

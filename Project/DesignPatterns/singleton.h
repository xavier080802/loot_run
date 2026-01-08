#ifndef _SINGLETON_H_
#define _SINGLETON_H_

/*
Usage:
//Ensure "public" is there so that GetInstance is public.
class MyClass : public Singleton<MyClass> {
	//stuff
};

//In other location
MyClass* ref = MyClass:GetInstance();

*/

template <typename T>
class Singleton
{
public:
	//Get the singleton instance
	static T* GetInstance() {
		if (instance == nullptr)
			instance = new T();

		return instance;
	}

	//Destroy the singleton instance
	static void Destroy() {
		if (instance)
		{
			delete instance;
			instance = nullptr;
		}
	}

protected:
	Singleton() {};
	virtual ~Singleton() {};

private:
	static T* instance;
};

template <typename T>
T* Singleton<T>::instance = nullptr;

#endif // !_SINGLETON_H_

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
		if (s_mInstance == nullptr)
			s_mInstance = new T();

		return s_mInstance;
	}

	//Destroy the singleton instance
	static void Destroy() {
		if (s_mInstance)
		{
			delete s_mInstance;
			s_mInstance = nullptr;
		}
	}

protected:
	Singleton() {};
	virtual ~Singleton() {};

private:
	static T* s_mInstance;
};

template <typename T>
T* Singleton<T>::s_mInstance = nullptr;

#endif // !_SINGLETON_H_

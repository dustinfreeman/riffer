#ifndef RFR_PARAM
#define RFR_PARAM

namespace rfr {
	
	struct AbstractParam {
		std::string name;
		int length; //only used by parameters of non-determinant length.
		virtual ~AbstractParam() {}
		virtual int get_type_id()=0;
	};

	template <class T>
	struct Param : AbstractParam {
		T value;
		Param(std::string _name, T _value, int _length = 0)
		{	
			name = _name;
			length = _length;
			set_value(_value);
		}
		virtual ~Param();

		int get_type_id();
		
		void set_value(T _value);

		T get_value() {
			return value;
		}
	};

	//below using template function specialisations
	//http://stackoverflow.com/questions/8220045/switch-template-type

	//set_value ===============================
	//generally, every set_value is a simple =
	// however, const values must be copied, since they are just being held on the stack.
	template<>
	void Param<int>::set_value(int _value) {
		value = _value;
	}
	template<>
	void Param<__int64>::set_value(__int64 _value) {
		value = _value;
	}
	template<>
	void Param<float>::set_value(float _value) {
		value = _value;
	}
	template<>
	void Param<char*>::set_value(char* _value) {
		value = _value;
	}
	template<>
	void Param<unsigned char*>::set_value(unsigned char* _value) {
		value = _value;
	}
	template<>
	void Param<void*>::set_value(void* _value) {
		value = _value;
	}
	template<>
	void Param<const char*>::set_value(const char* _value) {
		length = strlen(_value);
		//above will override of any given value of length;
		value = (const char*)malloc(length);
		memcpy((void*)value, (void*)_value, length);
	}

	//get_type_id ===============================
	template<>
	int Param<int>::get_type_id() {
		return INT_TYPE;
	}
	//I feel like I'm making a mess of the 64-bit ints.
	template<> //long
	int Param<int64_t>::get_type_id() {
		return INT_64_TYPE;
	}
    
	//template<> //long
	//int Param<long>::get_type_id() {
	//	return INT_64_TYPE;
	//} //OSX complained when this wasn't present.

	template<>
	int Param<char*>::get_type_id() {
		return CHAR_PTR_TYPE;
	}
	template<>
	int Param<const char*>::get_type_id() {
		return CHAR_PTR_TYPE;
	}
	template<>
	int Param<void*>::get_type_id() {
		return CHAR_PTR_TYPE;
	}

	//destructors ===============================
	template<>
	Param<char*>::~Param() {
		delete[] value;
	}
	template<>
	Param<const char*>::~Param() {
		free((void*)value);
	}

	template<>
	Param<void*>::~Param() {
		delete[] value;
	}

	template<>
	Param<__int64>::~Param() { }
	template<>
	Param<int>::~Param() { }
	/*template<>
	Param<float>::~Param() { }*/

};

#endif

//USAGE:
//rfr::Param<char*>("image", image_bytes)

#ifndef RFR_PARAM
#define RFR_PARAM

#include <stdint.h>

#include <string>

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
		virtual ~Param() { }

		virtual int get_type_id() {
            return UNDEFN_TYPE;
        }
		
		void set_value(T _value) {
            value = _value;
        }

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
	inline void Param<const char*>::set_value(const char* _value) {
		length = (int)strlen(_value);
		//above will override of any given value of length;
		value = (const char*)malloc(length);
		memcpy((void*)value, (void*)_value, length);
	}

	template<>
	inline void Param<std::string>::set_value(std::string _value) {
		length = _value.length();
		//above will override of any given value of length;
		value = _value;
	}
    
	//get_type_id ===============================
    template<>
	inline int Param<bool>::get_type_id() {
		return BOOL_TYPE;
	}
	template<>
	inline int Param<int>::get_type_id() {
		return INT_TYPE;
	}
	//I feel like I'm making a mess of the 64-bit ints.
	template<> //long
	inline int Param<long>::get_type_id() {
		return INT_64_TYPE;
	}
    template<> //long
	inline int Param<int64_t>::get_type_id() {
		return INT_64_TYPE;
	}
    
    template<>
	inline int Param<float>::get_type_id() {
		return FLOAT_TYPE;
	}
    
	//template<> //long
	//int Param<long>::get_type_id() {
	//	return INT_64_TYPE;
	//} //OSX complained when this wasn't present.

	template<>
	inline int Param<char*>::get_type_id() {
		return CHAR_PTR_TYPE;
	}
	template<>
	inline int Param<const char*>::get_type_id() {
		return CHAR_PTR_TYPE;
	}
	template<>
	inline int Param<void*>::get_type_id() {
		return CHAR_PTR_TYPE;
	}
    template<>
	inline int Param<std::string>::get_type_id() {
		return STRING_TYPE;
	}

	//destructors ===============================
	template<>
	inline Param<char*>::~Param() {
		delete[] value;
    }
	template<>
	inline Param<const char*>::~Param() {
		free((void*)value);
    }
	template<>
	inline Param<void*>::~Param() {
		delete[] value;
	}

};

#endif

//USAGE:
//rfr::Param<char*>("image", image_bytes)

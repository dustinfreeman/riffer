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
			: value(_value)
		{	
			name = _name;	
			length = _length;
		}
		~Param() { }

		int get_type_id();
		
		void set_value(T _value) {
			value = _value;
		}

		T get_value() {
			return value;
		}
	};

	//using template function specialisations
	//http://stackoverflow.com/questions/8220045/switch-template-type

	template<>
	int Param<int>::get_type_id() {
		return INT_TYPE;
	}

	template<> //long
	int Param<int64_t>::get_type_id() {
		return INT_64_TYPE;
	}
    
    template<> //long
	int Param<long>::get_type_id() {
		return INT_64_TYPE;
	} //OSX complained when this wasn't present.

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

};

#endif

//USAGE:
//rfr::Param<char*>("image", image_bytes)

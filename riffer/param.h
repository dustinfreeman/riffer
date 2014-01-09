namespace rfr {


	struct AbstractParam {
		std::string name;
		virtual ~AbstractParam() {}
		virtual int get_type_id()=0;
	};

	template <class T>
	struct Param : AbstractParam {
		T value;
		Param(std::string _name, T _value)
			: value(_value)
		{	name == _name;	}
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

	template<>
	int Param<char*>::get_type_id() {
		return CHAR_PTR_TYPE;
	}

};


//USAGE:
//rfr::Param<char*>("image", image_bytes)

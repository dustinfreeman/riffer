namespace rfr {
#define INT_TYPE 0
#define LONG_TYPE 1
#define CHAR_TYPE 2
#define CHAR_PTR_TYPE 3

	struct AbstractParam {
		std::string name;
		AbstractParam(std::string _name) 
			: name(_name)
		{ }
		~AbstractParam() {}

		int get_type_id();

		template <class T>
		void set_value(T _value);

		template <class T>
		T get_value();
		
		template <class T>
		bool cmp_value(AbstractParam other) {
			return get_value() == other.get_value();
		}
	};

	template <class T>
	struct Param : AbstractParam {
		//std::string name;
		T value;
		Param(std::string _name, T _value)
			: AbstractParam(_name), value(_value)
		{ }
		~Param() { }
		
		void set_value(T _value) {
			value = _value;
		}

		T get_value() {
			return value;
		}
	};

	//using template function specialisations
	//http://stackoverflow.com/questions/8220045/switch-template-type

	//QUESTION: should I implement the template functions as AbstractParam:: or Param:: ?

	template<>
	bool AbstractParam::cmp_value<int>(AbstractParam other) {
		return this->get_value<int>() == other.get_value<int>();
	}
};


//USAGE:
//rfr::Param<char*>("image", image_bytes)

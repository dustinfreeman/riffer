namespace rfr {
	#define INT_TYPE 0
	#define LONG_TYPE 1
	#define CHAR_TYPE 2
	#define CHAR_PTR_TYPE 3

	struct AbstractParam {
		std::string name;
		virtual ~AbstractParam();

		virtual int get_type_id()=0;

		template <class T>
		virtual void set_value(T _value)=0;

		template <class T>
		virtual T get_value()=0;
		
	};

	template <class T>
	struct Param : AbstractParam {
		//std::string name;
		T value;
		Param(std::string _name, T _value)
			: name(_name), value(_value)
		{ }
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

	template<>
	int Param<long>::get_type_id() {
		return LONG_TYPE;
	}


	//QUESTION: should I implement the template functions as AbstractParam:: or Param:: ?

};


//USAGE:
//rfr::Param<char*>("image", image_bytes)

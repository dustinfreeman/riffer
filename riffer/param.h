namespace rfr {
	template <class T>
	struct Param {
		std::string name;
		T value;
		Param(std::string _name, T _value)
			: name(_name), value(_value)
		{ }
	};
};

//rfr::Param<char*>("image", image_bytes)

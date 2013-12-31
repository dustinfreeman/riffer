namespace rfr {
	struct Chunk {
		std::string tag; //the top-level tag

		std::list<Param> params; //list of parameters

		Chunk(std::string _tag) {
			tag = _tag;
		}

		template <class T>
		compare<T>(T value1, T value2) {
			return value1 == value2;
		}
		compare<char*>(char* value1, char* value2) {
			return byte_compare(value1, value2);
		}
		//does this template-fu work?

		bool operator== (const Chunk &other) {
			//compares tag and each parameter
			if (tag != other.tag)
				return false;

			//Awkward O(n^2) compare
			for (std::list<Param>::iterator it=params.begin(); it != params.end(); it++) {
				for (std::list<Param>::iterator it_other=params.begin(); it_other != params.end(); it_other++) {
					if ((*it).name = (*it_other).name) {
						compare<T>((*it).value, (*it_other).value);
						//possibly type errors above?
					}
				}
			}

			return true; //nothing wrong!
		}
		
		template <class T>
		bool AddParameter(Param<T> new_param) {
			//adds a given parameter to the list of parameters
			//if it exists already, overwrites

			bool already_exists = false;
			for (std::list<Param>::iterator it=params.begin(); it != params.end(); it++) {
				if ( (*it).name == new_param.name) {
					already_exists = true;
					(*it).value = new_param.value; 
					//above will possibly cause an error (probably a compile error) if types do not match
				}
			}

			if (!already_exists) {
				params.push_back(new_param);
			}

			return already_exists;
		}

		template <class T>
		T GetParameter<T>(std::string param_name) {
			for (std::list<Param>::iterator it=params.begin(); it != params.end(); it++) {
				if ( (*it).name == param_name) {
					return (*it).value;
				}
			}
			//if we got here, the parameter did not exist.
			return nullptr;
		}
	};
};

//chunk.AddParameter(rfr::Param<int>("height", height));

//assert(width == chunk_by_timestamp.GetParameter<int>("width"));

//Both chunk and chunk_by_index are Chunks.
//assert(chunk == chunk_by_index);
//overload equality operator to compare each component

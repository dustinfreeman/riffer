#include <list>

#include "param.h"

namespace rfr {
	struct Chunk {
		std::string tag; //the top-level tag

		std::list<AbstractParam> params; //list of parameters
		//QUESTION: can we have a list of objects with different templates types?

		Chunk(std::string _tag = "NULL") {
			tag = _tag;
		}
		//"NULL" tag means an undefined chunk.

		/*template <class T>
		compare<T>(T value1, T value2) {
			return value1 == value2;
		}
		compare<char*>(char* value1, char* value2) {
			return byte_compare(value1, value2);
		}*/
		//does this template-fu work?

		bool operator== (const Chunk &other) {
			//compares tag and each parameter
			if (tag != other.tag)
				return false;

			//Awkward O(n^2) compare
			for (std::list<AbstractParam>::iterator it=params.begin(); it != params.end(); it++) {
				int type_id = it->get_type_id();
				for (std::list<AbstractParam>::iterator it_other=params.begin(); it_other != params.end(); it_other++) {
					int other_type_id = it_other->get_type_id();
					if ((*it).name == (*it_other).name) {
						if (type_id != other_type_id)
							return false;
						switch(type_id) {
						case INT_TYPE:
							if (!(*it).cmp_value<int>(*it_other))
								return false;
						case LONG_TYPE:
							if (!(*it).cmp_value<long>(*it_other))
								return false;
						}
						

						//TODO call byte_compare for char* type parameter

						//compare<T>((*it).value, (*it_other).value);

					}
				}
			}

			return true; //nothing wrong!
		}
		
		template <class T>
		bool add_parameter(Param<T> new_param) {
			//adds a given parameter to the list of parameters
			//if it exists already, overwrites

			bool already_exists = false;
			for (std::list<AbstractParam>::iterator it=params.begin(); it != params.end(); it++) {
				if ( (*it).name == new_param.name) {
					already_exists = true;
					(*it).set_value(new_param.value); 
					//above will possibly cause an error (probably a compile error) if types do not match
				}
			}

			if (!already_exists) {
				params.push_back(new_param);
			}

			return already_exists;
		}

		template <class T>
		T get_parameter(std::string param_name) {
			for (std::list<AbstractParam>::iterator it=params.begin(); it != params.end(); it++) {
				if ( (*it).name == param_name) {
					return (*it).get_value<T>();
				}
			}
			//if we got here, the parameter did not exist.
			//return nullptr;
			return 0;
		}
	};
};

//chunk.AddParameter(rfr::Param<int>("height", height));

//assert(width == chunk_by_timestamp.GetParameter<int>("width"));

//Both chunk and chunk_by_index are Chunks.
//assert(chunk == chunk_by_index);
//overload equality operator to compare each component

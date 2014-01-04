#include <list>
#include <map>

#include "param.h"

//http://stackoverflow.com/a/8473603/2518451
template <typename Map>
bool map_compare (Map const &lhs, Map const &rhs) {
    // No predicate needed because there is operator == for pairs already.
    return lhs.size() == rhs.size()
        && std::equal(lhs.begin(), lhs.end(),
                      rhs.begin());
}

namespace rfr {
	struct Chunk {
		std::string tag; //the top-level tag
		std::map<std::string, std::shared_ptr<AbstractParam>> params; //list of parameters

		Chunk(std::string _tag = NULL_TAG) {
			tag = _tag;
		}

		template <class T>
		bool add_parameter_by_tag(std::string param_tag, T value) {
			//adds a given parameter to the list of parameters
			//if it exists already, overwrites
			bool already_exists = false;
			
			std::map<std::string, std::shared_ptr<AbstractParam>>::iterator it;
			it = params.find(param_tag);

			if (it != params.end()) {
				already_exists = true;
				Param<T>* ptr = reinterpret_cast<Param<T>*>(it->second.get());
				ptr->value = value;
			} else {
				//does not already exist
				std::shared_ptr<Param<T>> new_param(new Param<T>(param_tag, value));
				params[param_tag] = new_param;
			}

			return already_exists;
		}

		template <class T>
		bool add_parameter(std::string param_name, T value) {
			std::string param_tag = tags::get_tag(param_name);
			return add_parameter_by_tag<T>(param_tag, value);
		}

		template <class T>
		T* get_parameter_by_tag(const std::string param_tag) {
			std::map<std::string, std::shared_ptr<AbstractParam>>::iterator it;
			it = params.find(param_tag);

			if (it != params.end()) {
				Param<T>* ptr = reinterpret_cast<Param<T>*>(it->second.get());
				return &ptr->value;
			} else {
				return nullptr;
			}
		}

		template <class T>
		char* get_parameter_by_tag_as_char_ptr(const std::string param_tag, unsigned int* length);

		template <class T>
		T* get_parameter(const std::string param_name) {
			std::string param_tag = tags::get_tag(param_name);
			return get_parameter_by_tag<T>(param_tag);
		}

		bool operator== (const Chunk &other) {
			//compares tag and each parameter

			//checks top-level tag.
			if (tag != other.tag)
				return false;

			return map_compare(params, other.params);
			//return true; //nothing wrong!
		}
	};

	template <>
	char* Chunk::get_parameter_by_tag_as_char_ptr<int>(const std::string param_tag, unsigned int* length) {
		int* data_typed = get_parameter_by_tag<int>(param_tag);
		if (data_typed == nullptr)
			return nullptr;
		*length = sizeof(int);
		return reinterpret_cast<char*>(*data_typed);
	}
	
	template <>
	char* Chunk::get_parameter_by_tag_as_char_ptr<long>(const std::string param_tag, unsigned int* length) {
		long* data_typed = get_parameter_by_tag<long>(param_tag);
		if (data_typed == nullptr)
			return nullptr;
		*length = sizeof(long);
		return reinterpret_cast<char*>(*data_typed);
	}

	template <>
	char* Chunk::get_parameter_by_tag_as_char_ptr<char*>(const std::string param_tag, unsigned int* length) {
		char** data_typed = get_parameter_by_tag<char*>(param_tag);
		if (data_typed == nullptr)
			return nullptr;
		//the above line may be re-writeable.
		*length = strlen(*data_typed);
		return *data_typed;
	}
};

//chunk.AddParameter(rfr::Param<int>("height", height));

//assert(width == chunk_by_timestamp.GetParameter<int>("width"));

//Both chunk and chunk_by_index are Chunks.
//assert(chunk == chunk_by_index);
//overload equality operator to compare each component

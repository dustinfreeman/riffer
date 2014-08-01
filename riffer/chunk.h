#include <list>
#include <map>

#pragma once

#ifndef RFR_CHUNK
#define RFR_CHUNK

#include <memory> //to get shared_ptr

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
		std::map<std::string, std::shared_ptr<AbstractParam> > params; //list of parameters

		Chunk(std::string _tag_name = NULL_TAG) {
			tag = tags::get_tag(_tag_name);
		}

		//copy constructor
		//Chunk(const Chunk &other) {
		//	this->tag = other.tag;

		//	std::map<std::string, std::shared_ptr<AbstractParam> >::const_iterator params_it = other.params.begin();
		//	while (params_it != other.params.end()) {
		//		const std::string sub_tag = params_it->first;
		//		params[sub_tag] = other.params.at(sub_tag);
		//		params_it++;
		//	}
		//}

		~Chunk() {
			//delete each parameter
			params.clear();
		}

		template <class T>
		bool add_parameter_by_tag(std::string param_tag, T value, int length = 0) {
			//adds a given parameter to the list of parameters
			//if it exists already, overwrites
			bool already_exists = false;
			
			std::map<std::string, std::shared_ptr<AbstractParam> >::iterator it;
			it = params.find(param_tag);

			if (it != params.end()) {
				already_exists = true;
				Param<T>* ptr = reinterpret_cast<Param<T>*>(it->second.get());
				ptr->value = value;
				ptr->length = length;
			} else {
				//does not already exist
                //std::cout << "value adding: " << value << "\n";
				std::shared_ptr<Param<T> > new_param(new Param<T>(param_tag, value, length));
				params[param_tag] = new_param;
			}

			return already_exists;
		}

		template <class T>
		bool add_parameter(std::string param_name, T value, int length = 0) {
			std::string param_tag = tags::get_tag(param_name);
			return add_parameter_by_tag<T>(param_tag, value, length);
		}

		//for parameters that need a length/size.
		//template <class T>
		//bool add_parameter_by_tag(std::string param_tag, T value, int length) {
		//	//adds a given parameter to the list of parameters
		//	//if it exists already, overwrites
		//	bool already_exists = false;
		//	
		//	std::map<std::string, std::shared_ptr<AbstractParam> >::iterator it;
		//	it = params.find(param_tag);

		//	if (it != params.end()) {
		//		already_exists = true;
		//		Param<T>* ptr = reinterpret_cast<Param<T>*>(it->second.get());
		//		ptr->value = value;
		//		ptr->length = length;
		//	} else {
		//		//does not already exist
  //              //std::cout << "value adding: " << value << "\n";
		//		std::shared_ptr<Param<T> > new_param(new Param<T>(param_tag, value, length));
		//		params[param_tag] = new_param;
		//	}

		//	return already_exists;
		//}
		//template <class T>
		//bool add_parameter(std::string param_name, T value, int length) {
		//	std::string param_tag = tags::get_tag(param_name);
		//	return add_parameter_by_tag<T>(param_tag, value, length);
		//}

		template <class T>
		T* get_parameter_by_tag(const std::string param_tag, unsigned int* length) {
			std::map<std::string, std::shared_ptr<AbstractParam> >::iterator it;
			it = params.find(param_tag);

			if (it != params.end()) {
				Param<T>* ptr = reinterpret_cast<Param<T>*>(it->second.get());
				*length = ptr->length;
				return &ptr->value;
			} else {
				length = 0;
				return nullptr;
			}
		}

		template <class T>
		T* get_parameter_by_tag(const std::string param_tag) {
			unsigned int length; //unused;
			return get_parameter_by_tag<T>(param_tag, &length);
		}

		template <class T>
		T* get_parameter(const std::string param_name, unsigned int* length) {
			std::string param_tag = tags::get_tag(param_name);
			return get_parameter_by_tag<T>(param_tag);
		}

		template <class T>
		T* get_parameter(const std::string param_name) {
			unsigned int length; //unused;
			return get_parameter<T>(param_name, &length);
		}

		//for generic-type binary file writing.
		template <class T>
		const char* get_parameter_by_tag_as_char_ptr(const std::string param_tag, unsigned int* length);

		bool operator== (const Chunk &other) {
			//compares tag and each parameter
			//CURRENT NOT FUNCTIONAL

			//checks top-level tag.
			if (tag != other.tag)
				return false;

			return map_compare(params, other.params);
			//return true; //nothing wrong!
		}
	};

	template <>
	inline const char* Chunk::get_parameter_by_tag_as_char_ptr<int>(const std::string param_tag, unsigned int* length) {
		int* data_typed = get_parameter_by_tag<int>(param_tag);
		if (data_typed == nullptr)
			return nullptr;
		*length = sizeof(int);
		return reinterpret_cast<char*>(data_typed);
	}
	
	template <>
	inline const char* Chunk::get_parameter_by_tag_as_char_ptr<int64_t>(const std::string param_tag, unsigned int* length) {
		int64_t* data_typed = get_parameter_by_tag<int64_t>(param_tag);
		if (data_typed == nullptr)
			return nullptr;
		*length = sizeof(int64_t);
		return reinterpret_cast<char*>(data_typed);
	}

	template <>
	inline const char* Chunk::get_parameter_by_tag_as_char_ptr<char*>(const std::string param_tag, unsigned int* length) {
		char** data_typed = get_parameter_by_tag<char*>(param_tag, length); //will set length.
		if (data_typed == nullptr)
			return nullptr;
		return *data_typed;
	}

	template <>
	inline const char* Chunk::get_parameter_by_tag_as_char_ptr<std::string>(const std::string param_tag, unsigned int* length) {
		std::string* data_typed = get_parameter_by_tag<std::string>(param_tag, length); //will set length.
		if (data_typed == nullptr)
			return nullptr;
		return data_typed->c_str();
	}
};

#endif

//chunk.AddParameter(rfr::Param<int>("height", height));

//assert(width == chunk_by_timestamp.GetParameter<int>("width"));

//Both chunk and chunk_by_index are Chunks.
//assert(chunk == chunk_by_index);
//overload equality operator to compare each component

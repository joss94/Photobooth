// -----------------------------------------------------------------------------
//  Copyright (c) 2017-2020, IXBLUE.
// -----------------------------------------------------------------------------
//  N O T I C E
//
//  THIS MATERIAL IS CONSIDERED TO BE THE INTELLECTUAL PROPERTY OF IXBLUE.
// -----------------------------------------------------------------------------

#pragma once

#include <tensorflow/c/eager/c_api.h>
#include <vector>
#include <algorithm>
#include <numeric>
#include "Model.h"
#include "QString"

class Tensor 
{

public:

	Tensor(const Model& model, const TF_Output& oper);
	~Tensor();

	void clean();

	template<typename T> void set_data(std::vector<T>& new_data);
	template<typename T> void set_data(std::vector<T> new_data, const std::vector<int64_t>& new_shape);
	template<typename T> std::vector<T> get_data();
	void deduce_shape(const Model& model);

private:

	void error_check(bool condition, const QString& error);
	template <typename T> static TF_DataType deduce_type();

public:

	int64_t* _dims;
	TF_Tensor* val;
	TF_Output op;
	TF_DataType type;
	std::vector<int64_t> shape;
	std::vector<int64_t>* actual_shape = nullptr;
	void* data = nullptr;
	int flag;
};
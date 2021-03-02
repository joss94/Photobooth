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
#include <iostream>
#include <fstream>
#include "QString"

class Tensor;

class Model 
{

public:

	explicit Model(const QString&, float gpuReq);

	~Model();

	TF_SessionOptions* createSessionOptions(double percentage);

	void run(const std::vector<Tensor*>& inputs, const std::vector<Tensor*>& outputs);

	void run(Tensor *input, Tensor *output);

public:

	TF_Graph* graph;
	TF_Session* session;
	TF_Status* status;

	// Read a file from a string
	TF_Buffer* read(QString filename);

	bool status_check(bool throw_exc) const;
	void error_check(bool condition, QString error) const;
};
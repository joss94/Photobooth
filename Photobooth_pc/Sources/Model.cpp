// -----------------------------------------------------------------------------
//  Copyright (c) 2017-2020, IXBLUE.
// -----------------------------------------------------------------------------
//  N O T I C E
//
//  THIS MATERIAL IS CONSIDERED TO BE THE INTELLECTUAL PROPERTY OF IXBLUE.
// -----------------------------------------------------------------------------

#include "Model.h"
#include "Tensor.h"
#include <algorithm>

#include "QDebug"

Model::Model(const QString& model_filename, float gpuReq)
{
	this->status = TF_NewStatus();
	this->graph = TF_NewGraph();

	qDebug() << "TF Version : " << TF_Version();

	// Create the session.
	TF_SessionOptions* sess_opts = createSessionOptions(gpuReq);
	
	this->session = TF_NewSession(this->graph, sess_opts, this->status);
	TF_DeleteSessionOptions(sess_opts);
	this->status_check(true);

	// Create the graph
	TF_Buffer* def = read(model_filename);
	this->error_check(def != nullptr, "An error occurred reading the model");

	TF_ImportGraphDefOptions* graph_opts = TF_NewImportGraphDefOptions();
	//TF_ImportGraphDefOptionsSetDefaultDevice(graph_opts, "/device:CPU:0");
	TF_GraphImportGraphDef(this->graph, def, graph_opts, this->status);


	TF_DeleteImportGraphDefOptions(graph_opts);
	TF_DeleteBuffer(def);

	this->status_check(true);
}

Model::~Model() 
{
	// WARNING : The Tensorflow API does not free the GPU memory on a session close... Onlyl way to reset GPU RAM is to kill process
	TF_CloseSession(this->session, this->status);
	TF_DeleteSession(this->session, this->status);
	TF_DeleteGraph(this->graph);
	this->status_check(true);
	TF_DeleteStatus(this->status);
}

TF_SessionOptions* Model::createSessionOptions(double percentage) 
{
	TF_SessionOptions* options = TF_NewSessionOptions();

	// create a byte-array for the serialized ProtoConfig, set the mandatory bytes (first three and last four)
	uint8_t config[15] = { 0x32, 0xb, 0x9, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x20, 0x1, 0x38, 0x1 };

	// put the desired percentage to the config byte-array, from 3 to 10:
	uint8_t* bytes = reinterpret_cast<uint8_t*>(&percentage);
	for (int i = 0; i < sizeof(percentage); i++)
	{
		config[i + 3] = bytes[i];
	}

	TF_SetConfig(options, (void*)config, 15, status);
	status_check(false);

	return options;
}
		
TF_Buffer *Model::read(QString filename) 
{
	std::ifstream file(filename.toStdString(), std::ios::binary | std::ios::ate);

	// Error opening the file
	if (!file.is_open()) 
	{
		std::cerr << "Unable to open file: " << filename.toStdString() << std::endl;
		return nullptr;
	}

	// Cursor is at the end to get size
	auto size = file.tellg();

	// Move cursor to the beginning
	file.seekg(0, std::ios::beg);

	// Read
	auto data = new char[size];
	file.seekg(0, std::ios::beg);
	file.read(data, size);

	// Create tensorflow buffer from read data
	TF_Buffer* buffer = TF_NewBufferFromString(data, size);

	// Close file and remove data
	file.close();
	delete[] data;

	return buffer;
}

void Model::run(const std::vector<Tensor*>& inputs, const std::vector<Tensor*>& outputs) 
{
	this->error_check(std::all_of(inputs.begin(), inputs.end(), [](const Tensor* i) { return i->flag == 1; }),
		"Error: Not all elements from the inputs are full");

	this->error_check(std::all_of(outputs.begin(), outputs.end(), [](const Tensor* o) { return o->flag != -1; }),
		"Error: Not all outputs Tensors are valid");

	// Clean previous stored outputs
	std::for_each(outputs.begin(), outputs.end(), [](Tensor* o) {o->clean(); });

	// Get input operations
	std::vector<TF_Output> io(inputs.size());
	std::transform(inputs.begin(), inputs.end(), io.begin(), [](const Tensor* i) {return i->op; });

	// Get input values
	std::vector<TF_Tensor*> iv(inputs.size());
	std::transform(inputs.begin(), inputs.end(), iv.begin(), [](const Tensor* i) {return i->val; });

	// Get output operations
	std::vector<TF_Output> oo(outputs.size());
	std::transform(outputs.begin(), outputs.end(), oo.begin(), [](const Tensor* o) {return o->op; });

	// Prepare output recipients
	auto ov = new TF_Tensor*[outputs.size()];

	TF_SessionRun(this->session, nullptr, io.data(), iv.data(), (int)inputs.size(), oo.data(), ov, (int)outputs.size(), nullptr, 0, nullptr, this->status);
	if (this->status_check(false))
	{
		// Save results on outputs and mark as full
		for (int i = 0; i < outputs.size(); i++)
		{
			outputs[i]->val = ov[i];
			outputs[i]->flag = 1;
			outputs[i]->deduce_shape(*this);
		}
	}

	delete[] ov;
}

void Model::run(Tensor *input, Tensor *output)
{
	this->run(std::vector<Tensor*>({ input }), std::vector<Tensor*>({ output }));
}

bool Model::status_check(bool throw_exc) const 
{
	if (TF_GetCode(this->status) != TF_OK) 
	{
		qDebug() << "Error : " << TF_Message(status);
		if (throw_exc)
		{
			throw std::runtime_error(TF_Message(status));
		}
		else 
		{
			return false;
		}
	}
	return true;
}

void Model::error_check(bool condition, QString error) const 
{
	if (!condition)
	{
		throw std::runtime_error(error.toStdString());
	}
}
/*
 * ml-lib, a machine learning library for Max and Pure Data
 * Copyright (C) 2013 Carnegie Mellon University
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ml.h"
#include "GRT.h"

#include <flext.h>

namespace ml
{
    
    typedef enum mlp_mode_
    {
        MLP_MODE_REGRESSION,
        MLP_MODE_CLASSIFICATION
    }
    mlp_mode;
    
    class ml_mlp : ml_base
    {
        FLEXT_HEADER_S(ml_mlp, ml_base, setup);
        
    public:
        ml_mlp()
        {
            post("ml.mlp: Multilayer Perceptron based on the GRT library");
            
            trainingData.setInputAndTargetDimensions(2, 1);
            
            GRT::UINT numInputNeurons = trainingData.getNumInputDimensions();
            GRT::UINT numHiddenNeurons = 2;
            GRT::UINT numOutputNeurons = trainingData.getNumTargetDimensions();
            
            mlp.init(numInputNeurons, numHiddenNeurons, numOutputNeurons);
            
            mlp.setMaxNumEpochs( 100 );
            mlp.setMinChange( 1.0e-2 );
            mlp.setNumRandomTrainingIterations( 5 );
            mlp.setUseValidationSet( true );
            mlp.setValidationSetSize( 20 );
            mlp.setRandomiseTrainingOrder( true );
            mlp.enableScaling( true );
            
        }
        
        ~ml_mlp()
        {
            
        }
        
    protected:
        static void setup(t_classid c)
        {
            FLEXT_CADDATTR_SET(c, "mode", set_mode);
            FLEXT_CADDATTR_SET(c, "num_inputs", set_num_inputs);
            FLEXT_CADDATTR_SET(c, "num_outputs", set_num_outputs);
            
            FLEXT_CADDATTR_GET(c, "mode", get_mode);
            FLEXT_CADDATTR_GET(c, "num_inputs", get_num_inputs);
            FLEXT_CADDATTR_GET(c, "num_outputs", get_num_outputs);
        }
        
        // Methods
        virtual void add(int argc, const t_atom *argv);
        virtual void save(const t_symbol *path) const;
        virtual void load(const t_symbol *path);
        virtual void normalize();
        virtual void train();
        virtual void clear();
        virtual void classify(int argc, const t_atom *argv);
        virtual void usage();
        
        // Attribute Setters
        void set_mode(int mode);
        void set_num_inputs(int num_inputs);
        void set_num_outputs(int num_outputs);
        
        // Attribute Getters
        void get_mode(int &mode) const;
        void get_num_inputs(int &num_inputs) const;
        void get_num_outputs(int &num_outputs) const;
        
        // TODO: set / get the stuff currently in the constructor
        
        // Attribute Getters
        
    private:
        // Method wrappers

        
        // Attribute wrappers
        FLEXT_CALLVAR_I(get_mode, set_mode);
        FLEXT_CALLVAR_I(get_num_inputs, set_num_inputs);
        FLEXT_CALLVAR_I(get_num_outputs, set_num_outputs);

        // Instance variables
        GRT::MLP mlp;
        GRT::LabelledRegressionData trainingData;
        mlp_mode mode;
    };
    
    // Utility functions
    std::string get_symbol_as_string(const t_symbol *symbol)
    {
        const char *c_string = flext::GetAString(symbol);
        std::string cpp_string;
        
        if (c_string == NULL)
        {
            error("symbol was NULL");
        }
        else
        {
            cpp_string.assign(c_string);
        }
        
        return cpp_string;
    }
    
    // Attribute setters
    void ml_mlp::set_mode(int mode)
    {
        if (mode > 1)
        {
            error("mode must be either 0 for regression or 1 for classification");
            return;
        }
        this->mode = (mlp_mode)mode;
    }
    
    void ml_mlp::set_num_inputs(int num_inputs)
    {
        if (num_inputs < 0)
        {
            error("number of inputs must be greater than zero");
        }
        
        GRT::UINT numHiddenNeurons = mlp.getNumHiddenNeurons();
        GRT::UINT numOutputNeurons = mlp.getNumOutputNeurons();
        
        trainingData.clear();
        
        bool success = trainingData.setInputAndTargetDimensions(num_inputs, numOutputNeurons);
        
        if (success == false)
        {
            error("unable to set input and target dimensions");
            return;
        }
        
        success = mlp.init(num_inputs, numHiddenNeurons, numOutputNeurons);

        if (success == false)
        {
            error("unable to initialise MLP");
        }
    }
    
    void ml_mlp::set_num_outputs(int num_outputs)
    {
        if (num_outputs < 0)
        {
            error("number of outputs must be greater than zero");
        }
        
        GRT::UINT numHiddenNeurons = mlp.getNumHiddenNeurons();
        GRT::UINT numInputNeurons = mlp.getNumInputNeurons();
        
        trainingData.clear();
        
        bool success = trainingData.setInputAndTargetDimensions(numInputNeurons, num_outputs);
        
        if (success == false)
        {
            error("unable to set input and target dimensions");
            return;
        }
        
        success = mlp.init(numInputNeurons, numHiddenNeurons, num_outputs);
        
        if (success == false)
        {
            error("unable to initialise MLP");
        }
    }

    // Attribute getters
    void ml_mlp::get_mode(int &mode) const
    {
        mode = this->mode;
    }
    
    void ml_mlp::get_num_inputs(int &num_inputs) const
    {
        num_inputs = mlp.getNumInputNeurons();
    }
    
    void ml_mlp::get_num_outputs(int &num_outputs) const
    {
        num_outputs = mlp.getNumOutputDimensions();
    }
    
    // Methods
    void ml_mlp::add(int argc, const t_atom *argv)
    {
        
        GRT::UINT numInputNeurons = mlp.getNumInputNeurons();
        GRT::UINT numOutputNeurons = mlp.getNumOutputNeurons();
        GRT::UINT combinedVectorSize = numInputNeurons + numOutputNeurons;
        
        // TODO: also check if trainingData inputDimensions / outpuDimensions matches?
        
        if (argc < 0 || (unsigned)argc != combinedVectorSize)
        {
            error("invalid input length, expected %d, got %d", combinedVectorSize, argc);
            return;
        }
        
        if (numInputNeurons != trainingData.getNumInputDimensions() || numOutputNeurons != trainingData.getNumTargetDimensions())
        {
            error("inconsistent MLP dimensions. To set, use attributes: num_input_neurons, num_output_neurons");
            return;
        }
        
        GRT::VectorDouble inputVector(numInputNeurons);
        GRT::VectorDouble targetVector(numOutputNeurons);
        
        for (uint32_t index = 0; index < (unsigned)argc; ++index)
        {
            float value = GetAFloat(argv[index]);

            if (index < numOutputNeurons)
            {
                targetVector[index] = value;
            }
            else
            {
                inputVector[index - numOutputNeurons] = value;
            }
        
        }

        trainingData.addSample(inputVector, targetVector);
    }

    void ml_mlp::save(const t_symbol *path) const
    {
        if (trainingData.getNumSamples() == 0)
        {
            error("no observations added, use 'add' to add training data");
            return;
        }
        
        std::string file_path = get_symbol_as_string(path);
        
        if (file_path.empty())
        {
            error("path string is empty");
            return;
        }
        
        bool success = trainingData.saveDatasetToFile(file_path);
        
        if (!success)
        {
            error("unable to save training data to path: %s", file_path.c_str());
        }
    }
    
    void ml_mlp::load(const t_symbol *path)
    {
        std::string file_path = get_symbol_as_string(path);
        
        if (file_path.empty())
        {
            error("path string is empty");
            return;
        }

        bool success = trainingData.loadDatasetFromFile(file_path);
        
        if (!success)
        {
            error("unable to load training data from path: %s", file_path.c_str());
        }
    }
    
    void ml_mlp::normalize()
    {
        error("function not implemented");
    }
    
    void ml_mlp::train()
    {
        if (trainingData.getNumSamples() == 0)
        {
            error("no observations added, use 'add' to add training data");
            return;
        }
        
        bool success = mlp.train(trainingData);
        
        if (!success)
        {
            error("training failed");
            return;
        }
        
        t_atom a_num_classes;
        
        SetInt(a_num_classes, mlp.getNumClasses());
        ToOutAnything(1, s_train, 1, &a_num_classes);
    }
    
    void ml_mlp::clear()
    {
        t_atom status;
        SetBool(status, true);
        
        trainingData.clear();
        
        ToOutAnything(1, s_cleared, 1, &status);
    }
    
    void ml_mlp::classify(int argc, const t_atom *argv)
    {
        if (trainingData.getNumSamples() == 0)
        {
            error("no observations added, use 'add' to add training data");
            return;
        }
        
        if (mlp.getTrained() == false)
        {
            error("model has not been trained, use 'train' to train the model");
            return;
        }
        
        GRT::UINT numInputNeurons = mlp.getNumInputNeurons();
        GRT::VectorDouble query(numInputNeurons);
        
        if (argc < 0 || (unsigned)argc != numInputNeurons)
        {
            error("invalid input length, expected %d, got %d", numInputNeurons, argc);
        }

        for (uint32_t index = 0; index < (uint32_t)argc; ++index)
        {
            double value = GetAFloat(argv[index]);
            query[index] = value;
        }
        
        bool success = mlp.predict(query);
        
        if (success == false)
        {
            error("unable to classify input");
            return;
        }
        
        GRT::VectorDouble regressionData = mlp.getRegressionData();
        GRT::VectorDouble::size_type numOutputDimensions = regressionData.size();
        
        if (numOutputDimensions != mlp.getNumOutputNeurons())
        {
            error("invalid output dimensions: %d", numOutputDimensions);
            return;
        }
        
        AtomList result;
        
        for (uint32_t index = 0; index < numOutputDimensions; ++index)
        {
            t_atom value_a;
            double value = regressionData[index];
            SetFloat(value_a, value);
            result.Append(value_a);
        }
        
        ToOutList(0, result);
        
    }
    
    void ml_mlp::usage()
    {
        error("function not implemented");
    }

    
    
    FLEXT_LIB("ml.mlp", ml_mlp);
    
} //namespace ml

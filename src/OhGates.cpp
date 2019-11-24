#include "plugin.hpp"


struct OhGates : Module {
	enum ParamIds {
		BUFFER_SIZE_PARAM,
		GATE_LENGTH_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		GATE_IN_INPUT,
		RESET_INPUT,
		CV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATE_OUT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	dsp::SchmittTrigger trigger_in;
	dsp::SchmittTrigger reset_in;
	int current = 0;
	int num_triggers = 1;

	enum Status {
		OFF,
		GATE_ON,
		NUM_STATUS
	};

	enum Events {
		TURNED_ON,
		TURNED_OFF,
		NUM_EVENTS
	};

	int status = OFF;
	bool trigger_on = false;
	int remaining_samples = 0;
	int sampleRate = 0;
	int buff_size = 1;

	OhGates() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(BUFFER_SIZE_PARAM, 0, 10, 0, "");
		configParam(GATE_LENGTH_PARAM, 20, 1000, 20, "");
	}

	void process(const ProcessArgs& args) override {
		if (inputs[GATE_IN_INPUT].isConnected()) {
			trigger_in.process(inputs[GATE_IN_INPUT].getVoltage());

			this->update_trigger(trigger_in.isHigh());	
		} else {
			this->current = 0;
			this->status = OFF;
		}

		if (inputs[RESET_INPUT].isConnected()) {
			reset_in.process(inputs[RESET_INPUT].getVoltage());
			if (reset_in.isHigh()) {
				this->current = 0;
			}
		}
		if (buff_size != this->get_num_gates_to_buffer()) {
			this->current = 0;
			buff_size = this->get_num_gates_to_buffer();
		}

		if (this->remaining_samples > 0) {
			this->remaining_samples--;
		}
		if (this->remaining_samples == 0) {
			this->shut_gate();		
		}
		this->sampleRate = args.sampleRate;
	}

	void fire_event(int event) {
		if (event == TURNED_ON) {
			this->current++;
			if (current == this->get_num_gates_to_buffer()) {
				this->fire_gate();
			}
		}
	}

	int get_num_gates_to_buffer()
	{
		float val = params[BUFFER_SIZE_PARAM].getValue();
		return (int)val;
	}

	float get_gate_time()
	{
		float val = params[GATE_LENGTH_PARAM].getValue();
		if (inputs[CV_INPUT].isConnected()) {
			val *= inputs[CV_INPUT].getVoltage()/10.0f;
		}

		if (val < 20) {
			val = 20;
		}

		return val/1000;
	}

	void fire_gate() {
		this->remaining_samples = this->sampleRate * this->get_gate_time();
		// INFO("remaining samples: %d", this->remaining_samples);
		// INFO("gate time: %.2f", this->get_gate_time());
		// INFO("CV IN: %.2f", inputs[CV_INPUT].getVoltage()/10.0f);
		outputs[GATE_OUT_OUTPUT].setVoltage(10.0f);
		this->current = 0;
	}

	void shut_gate() {
		outputs[GATE_OUT_OUTPUT].setVoltage(0.0f);
		this->remaining_samples = 0;
	}

	void update_trigger(bool on) {
		if (on != this->trigger_on) {
			// INFO("updating status");
			// INFO("TRIGGER ON: %d", this->trigger_on);
			if (on) {
				this->fire_event(TURNED_ON);
			} else {
				this->fire_event(TURNED_OFF);
			}
		}
		this->trigger_on = on;
	}
};


struct OhGatesWidget : ModuleWidget {
	OhGatesWidget(OhGates* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/OhGates2.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(15.24, 28.063)), module, OhGates::BUFFER_SIZE_PARAM));
		addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(15.24, 53.50)), module, OhGates::GATE_LENGTH_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.00, 77.061)), module, OhGates::GATE_IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.80, 77.061)), module, OhGates::RESET_INPUT));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.00, 100.713)), module, OhGates::CV_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(21.80, 100.713)), module, OhGates::GATE_OUT_OUTPUT));
	}
};


Model* modelOhGates = createModel<OhGates, OhGatesWidget>("OhGates");
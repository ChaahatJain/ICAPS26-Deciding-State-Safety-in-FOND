import collections
from torch import nn
from torch.nn import functional


class NetworkOld(nn.Module):
    def __init__(self, input_size: int, hidden_layer_sizes: list, output_size: int):
        super().__init__()
        assert(len(hidden_layer_sizes) >= 1)
        self.fc_in = nn.Linear(input_size, hidden_layer_sizes[0])  # input features
        self.fc_out = nn.Linear(hidden_layer_sizes[-1], output_size)  # output layer, one neuron for each action

        fc_hidden_list = list()
        for (h1, h2) in zip(hidden_layer_sizes[0:], hidden_layer_sizes[1:]):
            fc_hidden_list.append(('fc_hidden_' + str(len(fc_hidden_list)), nn.Linear(h1, h2)))
        self.fc_hidden = nn.Sequential(collections.OrderedDict(fc_hidden_list))

    def forward(self, x):
        # input
        x = self.fc_in(x)
        x = functional.relu(x)
        # hidden
        for hidden_layer in self.fc_hidden:
            x = hidden_layer(x)
            x = functional.relu(x)

        return self.fc_out(x)


class Network(nn.Module):
    def __init__(self, input_size: int, hidden_layer_sizes: list, output_size: int):
        super().__init__()
        assert(len(hidden_layer_sizes) >= 1)

        forwarding_list = list()
        layer_sizes = [input_size] + hidden_layer_sizes + [output_size]
        for i in range(0, len(layer_sizes) - 1):
            forwarding_list.append(('forwarding_' + str(i) + "_" + str(i + 1), nn.Linear(layer_sizes[i], layer_sizes[i + 1])))
        self.forwarding_functions = nn.Sequential(collections.OrderedDict(forwarding_list))

    def forward(self, x):
        for forwarding_f in self.forwarding_functions[:-1]:
            x = functional.relu(forwarding_f(x))
        return self.forwarding_functions[-1](x)

    # to load NN learned via libtorch (c++ api)
    def load_from_pt(self, pt_model):
        target_dict = self.forwarding_functions.state_dict()
        for (name, values) in zip(pt_model.state_dict().keys(), pt_model.state_dict().values()):
            target_dict[name].data.copy_(values)

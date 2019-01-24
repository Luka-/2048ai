# Welcome to reinforcement learning for 2048

This project hyperparametrizes the n-tuple temporal difference learning approach for 2048, as initially described in http://ieeexplore.ieee.org/abstract/document/6932907/ . 

Board representation is based on https://github.com/nneonneo/2048-ai 

<strong> Feel free to visit http://www.smart2048.fun for my own deployment </strong> of this project, using the configuration given in this repo's config file (config.txt). 

## How to set up `config.txt`

You choose how many training iterations you want the program to run for, and then you list all n-tuples. Tiles are numbered canonically from 0-15 (i.e. row by row). 

Note that each n-tuple uses order of  16<sup>n</sup> bytes of memory times some constant depending on number of n-tuples. Running a couple of tuples of size 7 or larger is already in the order of gigabytes, so plan accordingly. 

## Results of the training process

Results are stored in `src/outvalues.txt`. <strong> Note: </strong> src/outvalues.txt gets overwritten, so back it up before re-running the program if you wish to preserve it.

src/outvalues.txt will contain as many lines as there are n-tuples, each line containing 16<sup>n</sup> floating values delimited by space, where n is size of the corresponding n-tuple. If your n-tuple contains tiles 2<sup>a<sub>1</sub></sup>, ... , 2<sup>a<sub>n</sub></sup> (where empty tile is 2<sup>0</sup>), then value at index 16<sup>(n-1)</sup>a<sub>1</sub> + ... + 16a<sub>2</sub> + a<sub>1</sub> will be the approximation of current state for that specific n-tuple. 

If `src/invalues.txt` is present, program will read in those values and start training from that point. Otherwise, it will initialize everything to 0. Make sure that, if src/invalues.txt is present, its size corresponds to what is declared in config.txt. The easiest way to do this is to use src/outvalues.txt as an input for your next training iteration. 


## How to run

Go to src folder and type `make` . Executable will be called `rl`. 

## Analyze performance

`data/perform.txt` contains scores of all games in the previous training session, one line space delimited integers. <br><strong> Note: </strong> again, this file gets overwritten by defualt. You can plot it by typing 
``` 
python plot.py
```
in `data` folder. You need to have python and matplotlib installed. Plotting uses exponentially decaying average. You can change the rate in plot.py file, by changing the value for rate variable (line 3): <br>
` line = 0.005 ` <br>
In general, the larger the rate, the more variance in the plot. 

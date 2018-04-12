# Project descrition

This project essentially provides two algorithms written in C++17 and called HFP-growth and IFP-growth.
Both algorithms are fast and scalable algorithms to compute entropic measures from dataset made of categorical features encoded as integers. 
They are related somehow to the frequent pattern mining algorithm called FP-growth.
More precisely:
- HFP-growth mines the feature subsets whose relative entropy (between 0 and 1) of their joint empirical distribution is higher than a given threshold hmax.
- IFP-growth mines the top-k reliable approximate functional dependencies (see references for more details)

A third algorithm HAPriori is provided as a baseline algorithm in order to assess the speed and scalability of HFP-growth.
HApriori solves the same problem than HFP-growth but proceeds differently. It is an adaptation of APriori that mines frequent itemsets using a levelwise pattern enumeration and pruning.

# License

All software are released under the GNU Public License V3.

# Requirements

The project supports GNU C++ compiler (g++) under Linux distribution (tested on Ubuntu 16.04).
Please note the following requirements prior to installation:
- g++7.0 or higher since some C++ source files use the lastest C++17 features.
- Boost since some projects like filesystem or program_options are used.
- A recent version of CMake in order to generate the makefile

Below is a script to install the required packages on an Ubuntu system:

```
sudo add-apt-repository ppa:jonathonf/gcc-7.1
sudo apt update
sudo apt install -y g++-7

sudo apt install -y cmake
sudo apt install -y libboost-all-dev

```

# Installation 

After having installed the required packages, get the project files from Github:

``` 
git clone https://github.com/P-Fred/hfp-growth
``` 

Then, install the software as follows.

```
cd hfp-growth/build
export CXX= /usr/bin/g++-7
cmake ..
sudo make install
cd ..
```

# Usage

Every program provides help to choose the right command line flags. Some datasets are provided in the right format in the data subdirectory.

Here are examples for each command:
```
cd data
HFP-growth --hmax 1 --input abalone.json
HAPriori --hmax 1 --input abalone.json
IFP-growth --K 20 --target -1 --alpha 1 --input abalone.json
```

Some useful remarks:
- Applications take as inputs categorical data formatted in JSON. Every data must be a list of pairs of integers. The first integer encodes the feature number and starts at 0. The second integer is the value of the feature. Outputs are displayed in the same JSON format. A single output is a pair of a pattern defined by a list of feature numbers and of a score (entropy for HFP-growth and HApriori, Reliable fraction of information for IFP-growth).
- HFP-growth and HAPriori require 1) a threshold hmax whose value is between 0 (null entropy) and 1 (full entropy of all features) 2) an input dataset as standard input or as a file (using --input flag)
- IFP-growth  requires 1) a target feature number (-1 refers to the last feature) 2) a number K of top-k patterns to mine 3) a coefficient alpha of relaxation. alpha = 1 corresponds to exact mining without approximation. 4) an input dataset as standard input or as a file (using --input flag)

# References

- For a description of FP-growth:
Han, Jiawei, Jian Pei, Yiwen Yin, et Runying Mao. « Mining Frequent Patterns without Candidate Generation: A Frequent-Pattern Tree Approach ». Data Min. Knowl. Discov. 8, nᵒ 1 (2004): 53–87.
- For a description of APriori:
Agrawal, Rakesh, et Ramakrishnan Srikant. « Fast Algorithms for Mining Association Rules in Large Databases ». In Proceedings of 20th International Conference on Very Large Data Bases, 487–499. Santiago de Chile, Chile: Morgan Kaufmann, 1994.
- For a complete definition of Reliable fraction of information:
Mandros, Panagiotis, Mario Boley, et Jilles Vreeken. « Discovering Reliable Approximate Functional Dependencies ». In Proceedings of the 23rd ACM SIGKDD International Conference on  Knowledge Discovery and Data Mining, 355‑63. Halifax, NS, Canada: ACM, 2017.


# Contact the author at

<a href="https://github.com/P-Fred">Frédéric Pennerath</a>,.

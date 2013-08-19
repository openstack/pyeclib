#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <xor_code.h>

#define NUM_12_6_COMBS 987
#define NUM_10_5_COMBS 120

int failure_combs_10_5[NUM_10_5_COMBS][4] = { {0,-1,-1,-1}, {1,-1,-1,-1}, {2,-1,-1,-1}, {3,-1,-1,-1}, {4,-1,-1,-1}, {5,-1,-1,-1}, {6,-1,-1,-1},
                             {7,-1,-1,-1}, {8,-1,-1,-1}, {9,-1,-1,-1}, {10,-1,-1,-1}, {11,-1,-1,-1}, {12,-1,-1,-1}, {13,-1,-1,-1}, 
                             {14,-1,-1,-1}, {0, 1,-1,-1}, {0, 2,-1,-1}, {0, 3,-1,-1}, {0, 4,-1,-1}, {0, 5,-1,-1}, {0, 6,-1,-1}, {0, 7,-1,-1}, 
                             {0, 8,-1,-1}, {0, 9,-1,-1}, {0, 10,-1,-1}, {0, 11,-1,-1}, {0, 12,-1,-1}, {0, 13,-1,-1}, {0, 14,-1,-1}, {1, 2,-1,-1}, 
                             {1, 3,-1,-1}, {1, 4,-1,-1}, {1, 5,-1,-1}, {1, 6,-1,-1}, {1, 7,-1,-1}, {1, 8,-1,-1}, {1, 9,-1,-1}, {1, 10,-1,-1}, {1, 11,-1,-1}, 
                             {1, 12,-1,-1}, {1, 13,-1,-1}, {1, 14,-1,-1}, {2, 3,-1,-1}, {2, 4,-1,-1}, {2, 5,-1,-1}, {2, 6,-1,-1}, {2, 7,-1,-1}, {2, 8,-1,-1}, 
                             {2, 9,-1,-1}, {2, 10,-1,-1}, {2, 11,-1,-1}, {2, 12,-1,-1}, {2, 13,-1,-1}, {2, 14,-1,-1}, {3, 4,-1,-1}, {3, 5,-1,-1}, {3, 6,-1,-1}, 
                             {3, 7,-1,-1}, {3, 8,-1,-1}, {3, 9,-1,-1}, {3, 10,-1,-1}, {3, 11,-1,-1}, {3, 12,-1,-1}, {3, 13,-1,-1}, {3, 14,-1,-1}, {4, 5,-1,-1}, 
                             {4, 6,-1,-1}, {4, 7,-1,-1}, {4, 8,-1,-1}, {4, 9,-1,-1}, {4, 10,-1,-1}, {4, 11,-1,-1}, {4, 12,-1,-1}, {4, 13,-1,-1}, {4, 14,-1,-1}, 
                             {5, 6,-1,-1}, {5, 7,-1,-1}, {5, 8,-1,-1}, {5, 9,-1,-1}, {5, 10,-1,-1}, {5, 11,-1,-1}, {5, 12,-1,-1}, {5, 13,-1,-1}, {5, 14,-1,-1}, 
                             {6, 7,-1,-1}, {6, 8,-1,-1}, {6, 9,-1,-1}, {6, 10,-1,-1}, {6, 11,-1,-1}, {6, 12,-1,-1}, {6, 13,-1,-1}, {6, 14,-1,-1}, {7, 8,-1,-1}, 
                             {7, 9,-1,-1}, {7, 10,-1,-1}, {7, 11,-1,-1}, {7, 12,-1,-1}, {7, 13,-1,-1}, {7, 14,-1,-1}, {8, 9,-1,-1}, {8, 10,-1,-1}, {8, 11,-1,-1}, 
                             {8, 12,-1,-1}, {8, 13,-1,-1}, {8, 14,-1,-1}, {9, 10,-1,-1}, {9, 11,-1,-1}, {9, 12,-1,-1}, {9, 13,-1,-1}, {9, 14,-1,-1}, {10, 11,-1,-1}, 
                             {10, 12,-1,-1}, {10, 13,-1,-1}, {10, 14,-1,-1}, {11, 12,-1,-1}, {11, 13,-1,-1}, {11, 14,-1,-1}, {12, 13,-1,-1}, {12, 14,-1,-1}, {13, 14,-1} };

int failure_combs_12_6[NUM_12_6_COMBS][4] = { {0,-1,-1,-1}, {1,-1,-1,-1}, {2,-1,-1,-1}, {3,-1,-1,-1}, {4,-1,-1,-1}, {5,-1,-1,-1}, {6,-1,-1,-1}, 
                             {7,-1,-1,-1}, {8,-1,-1,-1}, {9,-1,-1,-1}, {10,-1,-1,-1}, {11,-1,-1,-1}, {12,-1,-1,-1}, {13,-1,-1,-1}, 
                             {14,-1,-1,-1}, {15,-1,-1,-1}, {16,-1,-1,-1}, {17,-1,-1,-1},
                             {0, 1,-1,-1}, {0, 2,-1,-1}, {0, 3,-1,-1}, {0, 4,-1,-1}, {0, 5,-1,-1}, {0, 6,-1,-1}, {0, 7,-1,-1}, {0, 8,-1,-1}, {0, 9,-1,-1}, {0, 10,-1,-1}, {0, 11,-1,-1}, {0, 12,-1,-1}, 
                             {0, 13,-1,-1}, {0, 14,-1,-1}, {0, 15,-1,-1}, {0, 16,-1,-1}, {0, 17,-1,-1}, {1, 2,-1,-1}, {1, 3,-1,-1}, {1, 4,-1,-1}, {1, 5,-1,-1}, {1, 6,-1,-1}, {1, 7,-1,-1}, {1, 8,-1,-1}, 
                             {1, 9,-1,-1}, {1, 10,-1,-1}, {1, 11,-1,-1}, {1, 12,-1,-1}, {1, 13,-1,-1}, {1, 14,-1,-1}, {1, 15,-1,-1}, {1, 16,-1,-1}, {1, 17,-1,-1}, {2, 3,-1,-1}, {2, 4,-1,-1}, 
                             {2, 5,-1,-1}, 
                             {2, 6,-1,-1}, {2, 7,-1,-1}, {2, 8,-1,-1}, {2, 9,-1,-1}, {2, 10,-1,-1}, {2, 11,-1,-1}, {2, 12,-1,-1}, {2, 13,-1,-1}, {2, 14,-1,-1}, {2, 15,-1,-1}, {2, 16,-1,-1}, 
                             {2, 17,-1,-1}, 
                             {3, 4,-1,-1}, {3, 5,-1,-1}, {3, 6,-1,-1}, {3, 7,-1,-1}, {3, 8,-1,-1}, {3, 9,-1,-1}, {3, 10,-1,-1}, {3, 11,-1,-1}, {3, 12,-1,-1}, {3, 13,-1,-1}, {3, 14,-1,-1}, {3, 15,-1,-1}, 
                             {3, 16,-1,-1}, {3, 17,-1,-1}, {4, 5,-1,-1}, {4, 6,-1,-1}, {4, 7,-1,-1}, {4, 8,-1,-1}, {4, 9,-1,-1}, {4, 10,-1,-1}, {4, 11,-1,-1}, {4, 12,-1,-1}, {4, 13,-1,-1}, {4, 14,-1,-1}, 
                             {4, 15,-1,-1}, {4, 16,-1,-1}, {4, 17,-1,-1}, {5, 6,-1,-1}, {5, 7,-1,-1}, {5, 8,-1,-1}, {5, 9,-1,-1}, {5, 10,-1,-1}, {5, 11,-1,-1}, {5, 12,-1,-1}, {5, 13,-1,-1}, 
                             {5, 14,-1,-1}, 
                             {5, 15,-1,-1}, {5, 16,-1,-1}, {5, 17,-1,-1}, {6, 7,-1,-1}, {6, 8,-1,-1}, {6, 9,-1,-1}, {6, 10,-1,-1}, {6, 11,-1,-1}, {6, 12,-1,-1}, {6, 13,-1,-1}, {6, 14,-1,-1}, 
                             {6, 15,-1,-1}, 
                             {6, 16,-1,-1}, {6, 17,-1,-1}, {7, 8,-1,-1}, {7, 9,-1,-1}, {7, 10,-1,-1}, {7, 11,-1,-1}, {7, 12,-1,-1}, {7, 13,-1,-1}, {7, 14,-1,-1}, {7, 15,-1,-1}, {7, 16,-1,-1}, 
                             {7, 17,-1,-1}, 
                             {8, 9,-1,-1}, {8, 10,-1,-1}, {8, 11,-1,-1}, {8, 12,-1,-1}, {8, 13,-1,-1}, {8, 14,-1,-1}, {8, 15,-1,-1}, {8, 16,-1,-1}, {8, 17,-1,-1}, {9, 10,-1,-1}, {9, 11,-1,-1}, 
                             {9, 12,-1,-1}, 
                             {9, 13,-1,-1}, {9, 14,-1,-1}, {9, 15,-1,-1}, {9, 16,-1,-1}, {9, 17,-1,-1}, {10, 11,-1,-1}, {10, 12,-1,-1}, {10, 13,-1,-1}, {10, 14,-1,-1}, {10, 15,-1,-1}, {10, 16,-1,-1}, 
                             {10, 17,-1,-1}, {11, 12,-1,-1}, {11, 13,-1,-1}, {11, 14,-1,-1}, {11, 15,-1,-1}, {11, 16,-1,-1}, {11, 17,-1,-1}, {12, 13,-1,-1}, {12, 14,-1,-1}, {12, 15,-1,-1}, 
                             {12, 16,-1,-1}, 
                             {12, 17,-1,-1}, {13, 14,-1,-1}, {13, 15,-1,-1}, {13, 16,-1,-1}, {13, 17,-1,-1}, {14, 15,-1,-1}, {14, 16,-1,-1}, {14, 17,-1,-1}, {15, 16,-1,-1}, {15, 17,-1,-1}, {16, 17,-1,-1},
                             {0, 1, 2,-1}, {0, 1, 3,-1}, {0, 1, 4,-1}, {0, 1, 5,-1}, {0, 1, 6,-1}, {0, 1, 7,-1}, {0, 1, 8,-1}, {0, 1, 9,-1}, {0, 1, 10,-1}, {0, 1, 11,-1}, 
                             {0, 1, 12,-1}, {0, 1, 13,-1}, {0, 1, 14,-1}, {0, 1, 15,-1}, {0, 1, 16,-1}, {0, 1, 17,-1}, {0, 2, 3,-1}, {0, 2, 4,-1}, {0, 2, 5,-1}, {0, 2, 6,-1}, 
                             {0, 2, 7,-1}, {0, 2, 8,-1}, {0, 2, 9,-1}, {0, 2, 10,-1}, {0, 2, 11,-1}, {0, 2, 12,-1}, {0, 2, 13,-1}, {0, 2, 14,-1}, {0, 2, 15,-1}, {0, 2, 16,-1}, 
                             {0, 2, 17,-1}, {0, 3, 4,-1}, {0, 3, 5,-1}, {0, 3, 6,-1}, {0, 3, 7,-1}, {0, 3, 8,-1}, {0, 3, 9,-1}, {0, 3, 10,-1}, {0, 3, 11,-1}, {0, 3, 12,-1}, 
                             {0, 3, 13,-1}, {0, 3, 14,-1}, {0, 3, 15,-1}, {0, 3, 16,-1}, {0, 3, 17,-1}, {0, 4, 5,-1}, {0, 4, 6,-1}, {0, 4, 7,-1}, {0, 4, 8,-1}, {0, 4, 9,-1}, 
                             {0, 4, 10,-1}, {0, 4, 11,-1}, {0, 4, 12,-1}, {0, 4, 13,-1}, {0, 4, 14,-1}, {0, 4, 15,-1}, {0, 4, 16,-1}, {0, 4, 17,-1}, {0, 5, 6,-1}, {0, 5, 7,-1}, 
                             {0, 5, 8,-1}, {0, 5, 9,-1}, {0, 5, 10,-1}, {0, 5, 11,-1}, {0, 5, 12,-1}, {0, 5, 13,-1}, {0, 5, 14,-1}, {0, 5, 15,-1}, {0, 5, 16,-1}, {0, 5, 17,-1}, 
                             {0, 6, 7,-1}, {0, 6, 8,-1}, {0, 6, 9,-1}, {0, 6, 10,-1}, {0, 6, 11,-1}, {0, 6, 12,-1}, {0, 6, 13,-1}, {0, 6, 14,-1}, {0, 6, 15,-1}, {0, 6, 16,-1}, 
                             {0, 6, 17,-1}, {0, 7, 8,-1}, {0, 7, 9,-1}, {0, 7, 10,-1}, {0, 7, 11,-1}, {0, 7, 12,-1}, {0, 7, 13,-1}, {0, 7, 14,-1}, {0, 7, 15,-1}, {0, 7, 16,-1}, 
                             {0, 7, 17,-1}, {0, 8, 9,-1}, {0, 8, 10,-1}, {0, 8, 11,-1}, {0, 8, 12,-1}, {0, 8, 13,-1}, {0, 8, 14,-1}, {0, 8, 15,-1}, {0, 8, 16,-1}, {0, 8, 17,-1}, 
                             {0, 9, 10,-1}, {0, 9, 11,-1}, {0, 9, 12,-1}, {0, 9, 13,-1}, {0, 9, 14,-1}, {0, 9, 15,-1}, {0, 9, 16,-1}, {0, 9, 17,-1}, {0, 10, 11,-1}, {0, 10, 12,-1}, 
                             {0, 10, 13,-1}, {0, 10, 14,-1}, {0, 10, 15,-1}, {0, 10, 16,-1}, {0, 10, 17,-1}, {0, 11, 12,-1}, {0, 11, 13,-1}, {0, 11, 14,-1}, {0, 11, 15,-1}, 
                             {0, 11, 16,-1}, {0, 11, 17,-1}, {0, 12, 13,-1}, {0, 12, 14,-1}, {0, 12, 15,-1}, {0, 12, 16,-1}, {0, 12, 17,-1}, {0, 13, 14,-1}, {0, 13, 15,-1}, 
                             {0, 13, 16,-1}, {0, 13, 17,-1}, {0, 14, 15,-1}, {0, 14, 16,-1}, {0, 14, 17,-1}, {0, 15, 16,-1}, {0, 15, 17,-1}, {0, 16, 17,-1}, {1, 2, 3,-1}, 
                             {1, 2, 4,-1}, {1, 2, 5,-1}, {1, 2, 6,-1}, {1, 2, 7,-1}, {1, 2, 8,-1}, {1, 2, 9,-1}, {1, 2, 10,-1}, {1, 2, 11,-1}, {1, 2, 12,-1}, {1, 2, 13,-1}, 
                             {1, 2, 14,-1}, {1, 2, 15,-1}, {1, 2, 16,-1}, {1, 2, 17,-1}, {1, 3, 4,-1}, {1, 3, 5,-1}, {1, 3, 6,-1}, {1, 3, 7,-1}, {1, 3, 8,-1}, {1, 3, 9,-1}, {1, 3, 10,-1}, 
                             {1, 3, 11,-1}, {1, 3, 12,-1}, {1, 3, 13,-1}, {1, 3, 14,-1}, {1, 3, 15,-1}, {1, 3, 16,-1}, {1, 3, 17,-1}, {1, 4, 5,-1}, {1, 4, 6,-1}, {1, 4, 7,-1}, 
                             {1, 4, 8,-1}, {1, 4, 9,-1}, {1, 4, 10,-1}, {1, 4, 11,-1}, {1, 4, 12,-1}, {1, 4, 13,-1}, {1, 4, 14,-1}, {1, 4, 15,-1}, {1, 4, 16,-1}, {1, 4, 17,-1}, 
                             {1, 5, 6,-1}, {1, 5, 7,-1}, {1, 5, 8,-1}, {1, 5, 9,-1}, {1, 5, 10,-1}, {1, 5, 11,-1}, {1, 5, 12,-1}, {1, 5, 13,-1}, {1, 5, 14,-1}, {1, 5, 15,-1}, 
                             {1, 5, 16,-1}, {1, 5, 17,-1}, {1, 6, 7,-1}, {1, 6, 8,-1}, {1, 6, 9,-1}, {1, 6, 10,-1}, {1, 6, 11,-1}, {1, 6, 12,-1}, {1, 6, 13,-1}, {1, 6, 14,-1}, 
                             {1, 6, 15,-1}, {1, 6, 16,-1}, {1, 6, 17,-1}, {1, 7, 8,-1}, {1, 7, 9,-1}, {1, 7, 10,-1}, {1, 7, 11,-1}, {1, 7, 12,-1}, {1, 7, 13,-1}, {1, 7, 14,-1}, 
                             {1, 7, 15,-1}, {1, 7, 16,-1}, {1, 7, 17,-1}, {1, 8, 9,-1}, {1, 8, 10,-1}, {1, 8, 11,-1}, {1, 8, 12,-1}, {1, 8, 13,-1}, {1, 8, 14,-1}, {1, 8, 15,-1}, 
                             {1, 8, 16,-1}, {1, 8, 17,-1}, {1, 9, 10,-1}, {1, 9, 11,-1}, {1, 9, 12,-1}, {1, 9, 13,-1}, {1, 9, 14,-1}, {1, 9, 15,-1}, {1, 9, 16,-1}, {1, 9, 17,-1}, 
                             {1, 10, 11,-1}, {1, 10, 12,-1}, {1, 10, 13,-1}, {1, 10, 14,-1}, {1, 10, 15,-1}, {1, 10, 16,-1}, {1, 10, 17,-1}, {1, 11, 12,-1}, {1, 11, 13,-1}, 
                             {1, 11, 14,-1}, {1, 11, 15,-1}, {1, 11, 16,-1}, {1, 11, 17,-1}, {1, 12, 13,-1}, {1, 12, 14,-1}, {1, 12, 15,-1}, {1, 12, 16,-1}, {1, 12, 17,-1}, 
                             {1, 13, 14,-1}, {1, 13, 15,-1}, {1, 13, 16,-1}, {1, 13, 17,-1}, {1, 14, 15,-1}, {1, 14, 16,-1}, {1, 14, 17,-1}, {1, 15, 16,-1}, {1, 15, 17,-1}, 
                             {1, 16, 17,-1}, {2, 3, 4,-1}, {2, 3, 5,-1}, {2, 3, 6,-1}, {2, 3, 7,-1}, {2, 3, 8,-1}, {2, 3, 9,-1}, {2, 3, 10,-1}, {2, 3, 11,-1}, {2, 3, 12,-1}, 
                             {2, 3, 13,-1}, {2, 3, 14,-1}, {2, 3, 15,-1}, {2, 3, 16,-1}, {2, 3, 17,-1}, {2, 4, 5,-1}, {2, 4, 6,-1}, {2, 4, 7,-1}, {2, 4, 8,-1}, {2, 4, 9,-1}, {2, 4, 10,-1}, 
                             {2, 4, 11,-1}, {2, 4, 12,-1}, {2, 4, 13,-1}, {2, 4, 14,-1}, {2, 4, 15,-1}, {2, 4, 16,-1}, {2, 4, 17,-1}, {2, 5, 6,-1}, {2, 5, 7,-1}, {2, 5, 8,-1}, 
                             {2, 5, 9,-1}, {2, 5, 10,-1}, {2, 5, 11,-1}, {2, 5, 12,-1}, {2, 5, 13,-1}, {2, 5, 14,-1}, {2, 5, 15,-1}, {2, 5, 16,-1}, {2, 5, 17,-1}, {2, 6, 7,-1}, 
                             {2, 6, 8,-1}, {2, 6, 9,-1}, {2, 6, 10,-1}, {2, 6, 11,-1}, {2, 6, 12,-1}, {2, 6, 13,-1}, {2, 6, 14,-1}, {2, 6, 15,-1}, {2, 6, 16,-1}, {2, 6, 17,-1}, 
                             {2, 7, 8,-1}, {2, 7, 9,-1}, {2, 7, 10,-1}, {2, 7, 11,-1}, {2, 7, 12,-1}, {2, 7, 13,-1}, {2, 7, 14,-1}, {2, 7, 15,-1}, {2, 7, 16,-1}, {2, 7, 17,-1}, 
                             {2, 8, 9,-1}, {2, 8, 10,-1}, {2, 8, 11,-1}, {2, 8, 12,-1}, {2, 8, 13,-1}, {2, 8, 14,-1}, {2, 8, 15,-1}, {2, 8, 16,-1}, {2, 8, 17,-1}, {2, 9, 10,-1}, 
                             {2, 9, 11,-1}, {2, 9, 12,-1}, {2, 9, 13,-1}, {2, 9, 14,-1}, {2, 9, 15,-1}, {2, 9, 16,-1}, {2, 9, 17,-1}, {2, 10, 11,-1}, {2, 10, 12,-1}, {2, 10, 13,-1}, 
                             {2, 10, 14,-1}, {2, 10, 15,-1}, {2, 10, 16,-1}, {2, 10, 17,-1}, {2, 11, 12,-1}, {2, 11, 13,-1}, {2, 11, 14,-1}, {2, 11, 15,-1}, {2, 11, 16,-1}, 
                             {2, 11, 17,-1}, {2, 12, 13,-1}, {2, 12, 14,-1}, {2, 12, 15,-1}, {2, 12, 16,-1}, {2, 12, 17,-1}, {2, 13, 14,-1}, {2, 13, 15,-1}, {2, 13, 16,-1}, 
                             {2, 13, 17,-1}, {2, 14, 15,-1}, {2, 14, 16,-1}, {2, 14, 17,-1}, {2, 15, 16,-1}, {2, 15, 17,-1}, {2, 16, 17,-1}, {3, 4, 5,-1}, {3, 4, 6,-1}, {3, 4, 7,-1}, 
                             {3, 4, 8,-1}, {3, 4, 9,-1}, {3, 4, 10,-1}, {3, 4, 11,-1}, {3, 4, 12,-1}, {3, 4, 13,-1}, {3, 4, 14,-1}, {3, 4, 15,-1}, {3, 4, 16,-1}, {3, 4, 17,-1}, 
                             {3, 5, 6,-1}, {3, 5, 7,-1}, {3, 5, 8,-1}, {3, 5, 9,-1}, {3, 5, 10,-1}, {3, 5, 11,-1}, {3, 5, 12,-1}, {3, 5, 13,-1}, {3, 5, 14,-1}, {3, 5, 15,-1}, 
                             {3, 5, 16,-1}, {3, 5, 17,-1}, {3, 6, 7,-1}, {3, 6, 8,-1}, {3, 6, 9,-1}, {3, 6, 10,-1}, {3, 6, 11,-1}, {3, 6, 12,-1}, {3, 6, 13,-1}, {3, 6, 14,-1}, 
                             {3, 6, 15,-1}, {3, 6, 16,-1}, {3, 6, 17,-1}, {3, 7, 8,-1}, {3, 7, 9,-1}, {3, 7, 10,-1}, {3, 7, 11,-1}, {3, 7, 12,-1}, {3, 7, 13,-1}, {3, 7, 14,-1}, 
                             {3, 7, 15,-1}, {3, 7, 16,-1}, {3, 7, 17,-1}, {3, 8, 9,-1}, {3, 8, 10,-1}, {3, 8, 11,-1}, {3, 8, 12,-1}, {3, 8, 13,-1}, {3, 8, 14,-1}, {3, 8, 15,-1}, 
                             {3, 8, 16,-1}, {3, 8, 17,-1}, {3, 9, 10,-1}, {3, 9, 11,-1}, {3, 9, 12,-1}, {3, 9, 13,-1}, {3, 9, 14,-1}, {3, 9, 15,-1}, {3, 9, 16,-1}, {3, 9, 17,-1}, 
                             {3, 10, 11,-1}, {3, 10, 12,-1}, {3, 10, 13,-1}, {3, 10, 14,-1}, {3, 10, 15,-1}, {3, 10, 16,-1}, {3, 10, 17,-1}, {3, 11, 12,-1}, {3, 11, 13,-1}, 
                             {3, 11, 14,-1}, {3, 11, 15,-1}, {3, 11, 16,-1}, {3, 11, 17,-1}, {3, 12, 13,-1}, {3, 12, 14,-1}, {3, 12, 15,-1}, {3, 12, 16,-1}, {3, 12, 17,-1}, 
                             {3, 13, 14,-1}, {3, 13, 15,-1}, {3, 13, 16,-1}, {3, 13, 17,-1}, {3, 14, 15,-1}, {3, 14, 16,-1}, {3, 14, 17,-1}, {3, 15, 16,-1}, {3, 15, 17,-1}, 
                             {3, 16, 17,-1}, {4, 5, 6,-1}, {4, 5, 7,-1}, {4, 5, 8,-1}, {4, 5, 9,-1}, {4, 5, 10,-1}, {4, 5, 11,-1}, {4, 5, 12,-1}, {4, 5, 13,-1}, {4, 5, 14,-1}, 
                             {4, 5, 15,-1}, {4, 5, 16,-1}, {4, 5, 17,-1}, {4, 6, 7,-1}, {4, 6, 8,-1}, {4, 6, 9,-1}, {4, 6, 10,-1}, {4, 6, 11,-1}, {4, 6, 12,-1}, {4, 6, 13,-1}, 
                             {4, 6, 14,-1}, {4, 6, 15,-1}, {4, 6, 16,-1}, {4, 6, 17,-1}, {4, 7, 8,-1}, {4, 7, 9,-1}, {4, 7, 10,-1}, {4, 7, 11,-1}, {4, 7, 12,-1}, {4, 7, 13,-1}, 
                             {4, 7, 14,-1}, {4, 7, 15,-1}, {4, 7, 16,-1}, {4, 7, 17,-1}, {4, 8, 9,-1}, {4, 8, 10,-1}, {4, 8, 11,-1}, {4, 8, 12,-1}, {4, 8, 13,-1}, {4, 8, 14,-1}, 
                             {4, 8, 15,-1}, {4, 8, 16,-1}, {4, 8, 17,-1}, {4, 9, 10,-1}, {4, 9, 11,-1}, {4, 9, 12,-1}, {4, 9, 13,-1}, {4, 9, 14,-1}, {4, 9, 15,-1}, {4, 9, 16,-1}, 
                             {4, 9, 17,-1}, {4, 10, 11,-1}, {4, 10, 12,-1}, {4, 10, 13,-1}, {4, 10, 14,-1}, {4, 10, 15,-1}, {4, 10, 16,-1}, {4, 10, 17,-1}, {4, 11, 12,-1}, 
                             {4, 11, 13,-1}, {4, 11, 14,-1}, {4, 11, 15,-1}, {4, 11, 16,-1}, {4, 11, 17,-1}, {4, 12, 13,-1}, {4, 12, 14,-1}, {4, 12, 15,-1}, {4, 12, 16,-1}, 
                             {4, 12, 17,-1}, {4, 13, 14,-1}, {4, 13, 15,-1}, {4, 13, 16,-1}, {4, 13, 17,-1}, {4, 14, 15,-1}, {4, 14, 16,-1}, {4, 14, 17,-1}, {4, 15, 16,-1}, 
                             {4, 15, 17,-1}, {4, 16, 17,-1}, {5, 6, 7,-1}, {5, 6, 8,-1}, {5, 6, 9,-1}, {5, 6, 10,-1}, {5, 6, 11,-1}, {5, 6, 12,-1}, {5, 6, 13,-1}, {5, 6, 14,-1}, 
                             {5, 6, 15,-1}, {5, 6, 16,-1}, {5, 6, 17,-1}, {5, 7, 8,-1}, {5, 7, 9,-1}, {5, 7, 10,-1}, {5, 7, 11,-1}, {5, 7, 12,-1}, {5, 7, 13,-1}, {5, 7, 14,-1}, 
                             {5, 7, 15,-1}, {5, 7, 16,-1}, {5, 7, 17,-1}, {5, 8, 9,-1}, {5, 8, 10,-1}, {5, 8, 11,-1}, {5, 8, 12,-1}, {5, 8, 13,-1}, {5, 8, 14,-1}, {5, 8, 15,-1}, 
                             {5, 8, 16,-1}, {5, 8, 17,-1}, {5, 9, 10,-1}, {5, 9, 11,-1}, {5, 9, 12,-1}, {5, 9, 13,-1}, {5, 9, 14,-1}, {5, 9, 15,-1}, {5, 9, 16,-1}, {5, 9, 17,-1}, 
                             {5, 10, 11,-1}, {5, 10, 12,-1}, {5, 10, 13,-1}, {5, 10, 14,-1}, {5, 10, 15,-1}, {5, 10, 16,-1}, {5, 10, 17,-1}, {5, 11, 12,-1}, {5, 11, 13,-1}, 
                             {5, 11, 14,-1}, {5, 11, 15,-1}, {5, 11, 16,-1}, {5, 11, 17,-1}, {5, 12, 13,-1}, {5, 12, 14,-1}, {5, 12, 15,-1}, {5, 12, 16,-1}, {5, 12, 17,-1}, 
                             {5, 13, 14,-1}, {5, 13, 15,-1}, {5, 13, 16,-1}, {5, 13, 17,-1}, {5, 14, 15,-1}, {5, 14, 16,-1}, {5, 14, 17,-1}, {5, 15, 16,-1}, {5, 15, 17,-1}, 
                             {5, 16, 17,-1}, {6, 7, 8,-1}, {6, 7, 9,-1}, {6, 7, 10,-1}, {6, 7, 11,-1}, {6, 7, 12,-1}, {6, 7, 13,-1}, {6, 7, 14,-1}, {6, 7, 15,-1}, {6, 7, 16,-1}, 
                             {6, 7, 17,-1}, {6, 8, 9,-1}, {6, 8, 10,-1}, {6, 8, 11,-1}, {6, 8, 12,-1}, {6, 8, 13,-1}, {6, 8, 14,-1}, {6, 8, 15,-1}, {6, 8, 16,-1}, {6, 8, 17,-1}, 
                             {6, 9, 10,-1}, {6, 9, 11,-1}, {6, 9, 12,-1}, {6, 9, 13,-1}, {6, 9, 14,-1}, {6, 9, 15,-1}, {6, 9, 16,-1}, {6, 9, 17,-1}, {6, 10, 11,-1}, {6, 10, 12,-1}, 
                             {6, 10, 13,-1}, {6, 10, 14,-1}, {6, 10, 15,-1}, {6, 10, 16,-1}, {6, 10, 17,-1}, {6, 11, 12,-1}, {6, 11, 13,-1}, {6, 11, 14,-1}, {6, 11, 15,-1}, 
                             {6, 11, 16,-1}, {6, 11, 17,-1}, {6, 12, 13,-1}, {6, 12, 14,-1}, {6, 12, 15,-1}, {6, 12, 16,-1}, {6, 12, 17,-1}, {6, 13, 14,-1}, {6, 13, 15,-1}, 
                             {6, 13, 16,-1}, {6, 13, 17,-1}, {6, 14, 15,-1}, {6, 14, 16,-1}, {6, 14, 17,-1}, {6, 15, 16,-1}, {6, 15, 17,-1}, {6, 16, 17,-1}, {7, 8, 9,-1}, {7, 8, 10,-1}, 
                             {7, 8, 11,-1}, {7, 8, 12,-1}, {7, 8, 13,-1}, {7, 8, 14,-1}, {7, 8, 15,-1}, {7, 8, 16,-1}, {7, 8, 17,-1}, {7, 9, 10,-1}, {7, 9, 11,-1}, {7, 9, 12,-1}, 
                             {7, 9, 13,-1}, {7, 9, 14,-1}, {7, 9, 15,-1}, {7, 9, 16,-1}, {7, 9, 17,-1}, {7, 10, 11,-1}, {7, 10, 12,-1}, {7, 10, 13,-1}, {7, 10, 14,-1}, {7, 10, 15,-1}, 
                             {7, 10, 16,-1}, {7, 10, 17,-1}, {7, 11, 12,-1}, {7, 11, 13,-1}, {7, 11, 14,-1}, {7, 11, 15,-1}, {7, 11, 16,-1}, {7, 11, 17,-1}, {7, 12, 13,-1}, 
                             {7, 12, 14,-1}, {7, 12, 15,-1}, {7, 12, 16,-1}, {7, 12, 17,-1}, {7, 13, 14,-1}, {7, 13, 15,-1}, {7, 13, 16,-1}, {7, 13, 17,-1}, {7, 14, 15,-1}, 
                             {7, 14, 16,-1}, {7, 14, 17,-1}, {7, 15, 16,-1}, {7, 15, 17,-1}, {7, 16, 17,-1}, {8, 9, 10,-1}, {8, 9, 11,-1}, {8, 9, 12,-1}, {8, 9, 13,-1}, {8, 9, 14,-1}, 
                             {8, 9, 15,-1}, {8, 9, 16,-1}, {8, 9, 17,-1}, {8, 10, 11,-1}, {8, 10, 12,-1}, {8, 10, 13,-1}, {8, 10, 14,-1}, {8, 10, 15,-1}, {8, 10, 16,-1}, {8, 10, 17,-1}, 
                             {8, 11, 12,-1}, {8, 11, 13,-1}, {8, 11, 14,-1}, {8, 11, 15,-1}, {8, 11, 16,-1}, {8, 11, 17,-1}, {8, 12, 13,-1}, {8, 12, 14,-1}, {8, 12, 15,-1}, {8, 12, 16,-1}, 
                             {8, 12, 17,-1}, {8, 13, 14,-1}, {8, 13, 15,-1}, {8, 13, 16,-1}, {8, 13, 17,-1}, {8, 14, 15,-1}, {8, 14, 16,-1}, {8, 14, 17,-1}, {8, 15, 16,-1}, {8, 15, 17,-1}, 
                             {8, 16, 17,-1}, {9, 10, 11,-1}, {9, 10, 12,-1}, {9, 10, 13,-1}, {9, 10, 14,-1}, {9, 10, 15,-1}, {9, 10, 16,-1}, {9, 10, 17,-1}, {9, 11, 12,-1}, {9, 11, 13,-1}, 
                             {9, 11, 14,-1}, {9, 11, 15,-1}, {9, 11, 16,-1}, {9, 11, 17,-1}, {9, 12, 13,-1}, {9, 12, 14,-1}, {9, 12, 15,-1}, {9, 12, 16,-1}, {9, 12, 17,-1}, {9, 13, 14,-1}, 
                             {9, 13, 15,-1}, {9, 13, 16,-1}, {9, 13, 17,-1}, {9, 14, 15,-1}, {9, 14, 16,-1}, {9, 14, 17,-1}, {9, 15, 16,-1}, {9, 15, 17,-1}, {9, 16, 17,-1}, {10, 11, 12,-1}, 
                             {10, 11, 13,-1}, {10, 11, 14,-1}, {10, 11, 15,-1}, {10, 11, 16,-1}, {10, 11, 17,-1}, {10, 12, 13,-1}, {10, 12, 14,-1}, {10, 12, 15,-1}, {10, 12, 16,-1}, 
                             {10, 12, 17,-1}, {10, 13, 14,-1}, {10, 13, 15,-1}, {10, 13, 16,-1}, {10, 13, 17,-1}, {10, 14, 15,-1}, {10, 14, 16,-1}, {10, 14, 17,-1}, {10, 15, 16,-1}, 
                             {10, 15, 17,-1}, {10, 16, 17,-1}, {11, 12, 13,-1}, {11, 12, 14,-1}, {11, 12, 15,-1}, {11, 12, 16,-1}, {11, 12, 17,-1}, {11, 13, 14,-1}, {11, 13, 15,-1}, 
                             {11, 13, 16,-1}, {11, 13, 17,-1}, {11, 14, 15,-1}, {11, 14, 16,-1}, {11, 14, 17,-1}, {11, 15, 16,-1}, {11, 15, 17,-1}, {11, 16, 17,-1}, {12, 13, 14,-1}, 
                             {12, 13, 15,-1}, {12, 13, 16,-1}, {12, 13, 17,-1}, {12, 14, 15,-1}, {12, 14, 16,-1}, {12, 14, 17,-1}, {12, 15, 16,-1}, {12, 15, 17,-1}, {12, 16, 17,-1}, 
                             {13, 14, 15,-1}, {13, 14, 16,-1}, {13, 14, 17,-1}, {13, 15, 16,-1}, {13, 15, 17,-1}, {13, 16, 17,-1}, {14, 15, 16,-1}, {14, 15, 17,-1}, {14, 16, 17,-1}, {15, 16, 17,-1} };

void fill_buffer(unsigned char *buf, int size, int seed)
{
  int i;

  buf[0] = seed;

  for (i=1; i < size; i++) {
    buf[i] = ((buf[i-1] + i) % 256);
  }
}

int check_buffer(unsigned char *buf, int size, int seed)
{
  int i;

  if (buf[0] != seed) {
    fprintf(stderr, "Seed does not match index 0: %u\n", buf[0]);
    return -1;
  }

  for (i=1; i < size; i++) {
    if (buf[i] != ((buf[i-1] + i) % 256)) {
      fprintf(stderr, "Buffer does not match index %d: %u\n", i, (buf[i] & 0xff));
      return -1;
    }
  }

  return 0;
}

int test_hd_code(xor_code_t *code_desc, int num_failure_combs, int failure_combs[][4])
{
  int i;
  int num_iter = 1000;
  int blocksize = 32768;
  int missing_idxs[4] = { -1 };
  char **data, **parity;
  clock_t start_time, end_time;

  srand(time(NULL));

  data = (char**)malloc(code_desc->k * sizeof(char*));
  parity = (char**)malloc(code_desc->m * sizeof(char*));

  for (i=0; i < code_desc->k; i++) {
    data[i] = aligned_malloc(blocksize, 16);
    fill_buffer(data[i], blocksize, i);
    if (!data[i]) {
      fprintf(stderr, "Could not allocate memnory for data %d\n", i);
      exit(2);
    }
  }
  
  for (i=0; i < code_desc->m; i++) {
    parity[i] = aligned_malloc(blocksize, 16);
    memset(parity[i], 0, blocksize);
    if (!parity[i]) {
      fprintf(stderr, "Could not allocate memnory for parity %d\n", i);
      exit(2);
    }
  }
  
  start_time = clock();
  for (i=0; i < num_iter-1; i++) {
    code_desc->encode(code_desc, data, parity, blocksize);
  }
  end_time = clock();

  fprintf(stderr, "Encode: %.2f MB/s\n", ((double)(num_iter * blocksize * code_desc->k) / 1000 / 1000 ) / ((double)(end_time-start_time) / CLOCKS_PER_SEC));
  
  for (i=0; i < code_desc->m; i++) {
    memset(parity[i], 0, blocksize);
  }

  code_desc->encode(code_desc, data, parity, blocksize);
  
  for (i=0; i < num_failure_combs; i++) {
    int missing_idx_0 = failure_combs[i][0];
    int missing_idx_1 = failure_combs[i][1];
    int missing_idx_2 = failure_combs[i][2];

    missing_idxs[0] = missing_idx_0;
    missing_idxs[1] = missing_idx_1;
    missing_idxs[2] = missing_idx_2;
    missing_idxs[3] = -1;

    if (missing_idxs[0] > -1) {
      if (missing_idxs[0] < code_desc->k) {
        memset(data[missing_idxs[0]], 0, blocksize);
      } else {
        memset(parity[missing_idxs[0] - code_desc->k], 0, blocksize);
      }
    }
    if (missing_idxs[1] > -1) {
      if (missing_idxs[1] < code_desc->k) {
        memset(data[missing_idxs[1]], 0, blocksize);
      } else {
        memset(parity[missing_idxs[1] - code_desc->k], 0, blocksize);
      }
    }
    if (missing_idxs[2] > -1) {
      if (missing_idxs[2] < code_desc->k) {
        memset(data[missing_idxs[2]], 0, blocksize);
      } else {
        memset(parity[missing_idxs[2] - code_desc->k], 0, blocksize);
      }
    }

    code_desc->decode(code_desc, data, parity, missing_idxs, blocksize, 1);
  
    if (missing_idxs[0] > -1 && missing_idxs[0] < code_desc->k && check_buffer(data[missing_idx_0], blocksize, missing_idx_0) < 0) {
      fprintf(stderr, "Decode did not work: %d (%d %d %d)!\n", missing_idxs[0], missing_idxs[0], missing_idxs[1], missing_idxs[2]);
      exit(2);
    }
    if (missing_idxs[1] > -1 && missing_idxs[1] < code_desc->k && check_buffer(data[missing_idx_1], blocksize, missing_idx_1) < 0) {
      fprintf(stderr, "Decode did not work: %d (%d %d %d)!\n", missing_idxs[1], missing_idxs[0], missing_idxs[1], missing_idxs[2]);
      exit(2);
    }
    if (missing_idxs[2] > -1 && missing_idxs[2] < code_desc->k && check_buffer(data[missing_idx_2], blocksize, missing_idx_2) < 0) {
      fprintf(stderr, "Decode did not work: %d (%d %d %d)!\n", missing_idxs[2], missing_idxs[0], missing_idxs[1], missing_idxs[2]);
      exit(2);
    }
  }

  start_time = clock();
  for (i=0; i < num_iter; i++) {
    int j;

    missing_idxs[0] = rand() % (code_desc->k + code_desc->m);
    for (j=1; j < code_desc->hd-1;j++) {
      missing_idxs[j] = (missing_idxs[j-1] + 1) % (code_desc->k + code_desc->m);
    }
    missing_idxs[code_desc->hd-1] = -1;

    if (missing_idxs[0] > -1 && missing_idxs[0] < code_desc->k) {
      memset(data[missing_idxs[0]], 0, blocksize);
    }
    if (missing_idxs[1] > -1 && missing_idxs[1] < code_desc->k) {
      memset(data[missing_idxs[1]], 0, blocksize);
    }
    if (missing_idxs[2] > -1 && missing_idxs[2] < code_desc->k) {
      memset(data[missing_idxs[2]], 0, blocksize);
    }

    code_desc->decode(code_desc, data, parity, missing_idxs, blocksize, 1);
  }
  end_time = clock();
  
  fprintf(stderr, "Decode: %.2f MB/s\n", ((double)(num_iter * blocksize * code_desc->k) / 1000 / 1000 ) / ((double)(end_time-start_time) / CLOCKS_PER_SEC));

  return 0;
}

int main()
{
  int ret = 0;
  xor_code_t *code_desc;

  code_desc = init_xor_hd_code(12, 6, 4);
  ret = test_hd_code(code_desc, NUM_12_6_COMBS, failure_combs_12_6);
  free(code_desc);

  if (ret != 0) {
    return ret;
  }

  code_desc = init_xor_hd_code(10, 5, 3);
  ret = test_hd_code(code_desc, NUM_10_5_COMBS, failure_combs_10_5);
  free(code_desc);
  if (ret != 0) {
    return ret;
  }
}


# 02Q2-Shared memory across terminals: Assignment Prompts

Named shared memory can be accessed across processes on the same system, even if they do not share a common parent.

For this one, you will not spawn children processes, but instead open multiple terminal windows to run multiple instances of the program.

## Task description

Similar to part 2 question 1, you will write sychronization code to coordinate the instaces that mine the blocks, using semaphores and shared memories. Again, when an instance mines and attaches a block, it should signal other instances; upon receiving such signal, the other instances should stop mining the current block and prepare to mine the next block.

Since there is no common parent process, the process who first mined and attached a block will be responsible for generating the next block. The code for attaching a block and generating a new block has been included in the code template.
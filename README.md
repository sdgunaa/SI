# Shell Intelligence (SI) 

Shell Intelligence (SI) is an advanced LLM-powered agent designed to enhance users interaction with the Linux command-line interface (CLI). SI enables users to interact with Linux CLI using plain English instead of having to use complex commands.

As an intelligent agent, SI not only simplifies complex command execution but also offers context-aware assistance. It provides clear, user-friendly error explanations and actionable suggestions for troubleshooting. Additionally, SI transforms technical command outputs into plain English reports, ensuring users of all expertise levels can easily understand and leverage Linux functionalities.

## Getting Started

To use Shell Intelligence (SI), follow these steps:

##### Install SI on your Linux system.
`git clone https://github.com/guna-sd/SI.git`

run `cd SI`

run `make`

Start using by running `si`

## Example Usage

Here are some examples of how to use SI:

- Set the default text editor to "gedit":
```
guna@anug:[/home/guna/]$!> Set the default text editor to gedit.
```

- Configure the Linux firewall to allow all incoming TCP traffic on port 80:
```
guna@anug:[/home/guna/]$!> Configure the Linux firewall to allow all incoming TCP traffic on port 80.
```

- Generate a report of running processes:
```
guna@anug:[/home/guna/]$!> Generate a report of running processes.
```

> [!WARNING]
> While SI offers significant advantages in terms of usability and accessibility, it may have some limitations:

- **Language Understanding**: SI's natural language processing capabilities may not be perfect, leading to occasional misinterpretations of commands.

- **Context Sensitivity**: SI's understanding of context may be limited, resulting in inaccuracies when dealing with complex instructions.

- **Dependency on Language Models**: SI's performance relies on the underlying language model and updates to language models may impact its behavior.

#### Development
This project has been tested with pre-trained (base) Models which barely manages. Soon for further improvement and stability the SI will be using Llama3
specifically finetuned for SI with both supervised and Reinforcement Learning methods and quantized model for performance improvements.

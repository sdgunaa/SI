

export interface OutputLine {
  id: string;
  text: string;
  type: "stdout" | "stderr" | "info" | "success" | "error";
  timestamp?: string;
}

export interface Block {
  id: string;
  command: string;
  timestamp: string;
  directory: string;
  gitBranch?: string;
  output: OutputLine[];
  status: "success" | "error" | "running";
  duration?: string;
}

export const mockBlocks: Block[] = [
  {
    id: "1",
    command: "cd work-env/",
    timestamp: "10:00:23",
    directory: "~/work-env",
    output: [],
    status: "success",
    duration: "0.002s"
  },
  {
    id: "2",
    command: "git branch",
    timestamp: "10:00:25",
    directory: "~/work-env",
    gitBranch: "main",
    output: [
      { id: "o1", text: "  feature-459-fixed-menu", type: "stdout" },
      { id: "o2", text: "  feature-471-update-customer-api", type: "stdout" },
      { id: "o3", text: "  feature-631-update-customer-schema", type: "stdout" },
      { id: "o4", text: "  feature-782-update-footer", type: "stdout" },
      { id: "o5", text: "* main", type: "success" },
    ],
    status: "success",
    duration: "0.012s"
  },
  {
    id: "3",
    command: "./script/setup",
    timestamp: "10:01:45",
    directory: "~/warp-code/warp-server",
    gitBranch: "main",
    output: [
      { id: "e1", text: "Error: The setup script has failed due to the wrong version of Node.js.", type: "error" },
      { id: "e2", text: "The script output suggests installing Node.js version 18.19.1.", type: "info" },
    ],
    status: "error",
    duration: "2.174s"
  }
];

export const mockSuggestions = [
  { id: "s1", text: "git checkout -b feature/new-agent", description: "Create new branch" },
  { id: "s2", text: "npm run dev", description: "Start development server" },
  { id: "s3", text: "docker-compose up -d", description: "Start containers" },
];

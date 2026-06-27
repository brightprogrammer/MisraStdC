const { exec } = require("child_process");

const repositoryUrl = "https://github.com/zeon-studio/hugoplate";
const localDirectory = "./themes/hugoplate";
const foldersToFetch = ["assets", "layouts"];
const foldersToSkip = ["exampleSite"];

const excludePattern = foldersToSkip.join("|");

// Precompute safe, static command strings keyed by allowed folder name
const folderCommands = Object.fromEntries(
  foldersToFetch.map((f) => [
    f,
    `curl -L ${repositoryUrl}/tarball/main | tar -xz --strip-components=1 --directory=${localDirectory} --exclude=$(curl -sL ${repositoryUrl}/tarball/main | tar -tz | grep -E "/(${excludePattern})/") */${f}`,
  ]),
);

// Fetch each specified folder using only precomputed static commands
for (const cmd of Object.values(folderCommands)) {
  exec(cmd);
}

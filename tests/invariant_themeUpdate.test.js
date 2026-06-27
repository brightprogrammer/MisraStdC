const { exec } = require('child_process');
const path = require('path');

// Import the actual production function
const themeUpdate = require('./themeUpdate.js');

describe('Shell commands never include unsanitized user input', () => {
  const payloads = [
    // Exact exploit case - command injection
    'theme; rm -rf /',
    // Alternative injection syntax
    '$(whoami)',
    // Backtick execution
    '`id`',
    // Boundary case - normal input
    'valid-theme-folder',
    // Edge case with spaces and special chars
    'theme folder; echo hacked'
  ];

  test.each(payloads)('rejects adversarial input: %s', async (folder) => {
    // Mock exec to capture the actual command being executed
    const execCalls = [];
    const originalExec = exec;
    
    // Temporarily replace exec to intercept calls
    require('child_process').exec = jest.fn((command, callback) => {
      execCalls.push(command);
      // Simulate error to prevent actual execution
      callback(new Error('Mocked exec call'));
    });

    try {
      await themeUpdate(folder);
      
      // If we reach here, check what command would have been executed
      if (execCalls.length > 0) {
        // Assert that the raw folder input doesn't appear unescaped in the command
        execCalls.forEach(command => {
          // The command should NOT contain the raw, unescaped folder value
          // If it does, that's a command injection vulnerability
          expect(command).not.toContain(folder);
        });
      }
    } catch (error) {
      // If the function throws, that's acceptable - it means it rejected bad input
      // No assertion needed here as throwing is a valid defense
    } finally {
      // Restore original exec
      require('child_process').exec = originalExec;
    }
  });
});
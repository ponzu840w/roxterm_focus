# roxterm_focus
On antiX (and some other setups), when you open a second (or subsequent) Roxterm window while another application is active, focus does not move to the new window.

Reference: [New Roxterm / New window opens in background #254](https://github.com/realh/roxterm/issues/254)

## Reproduction Steps
1. Open Roxterm.  
2. Open another app (e.g. zzzFM).  
3. Press `CTRL+ALT+T` to open a second Roxterm window.  
4. Keyboard input still goes to zzzFM.

## Solution
This small C daemon runs in your desktop session and automatically activates any newly opened Roxterm window.

## Usage
```bash
sudo apt install libc6-dev libx11-dev # install dependencies
make
sudo make install
echo "/usr/local/bin/roxterm_focus &" >> ~/.desktop-session/startup
```

## Debug
```
make debug
./roxterm_focus_debug
```

or

```
make
sudo make install
mkdir -p "$HOME/.local/var/log"
cp debug_run.sh ~/.local/bin/
echo "~/.local/bin/debug_run.sh &" >> ~/.desktop-session/startup
```

check $HOME/.local/var/log/roxterm-forcus.log

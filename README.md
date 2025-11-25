<p align="center">
  <img style="cursor: default;" height="450" src="img/Untitled.png">
</p>

# Plants vs. Zombies: Game of the Year - Game Trainer

### Summary
> Game trainers are programs made to modify memory of a computer game
thereby modifying its behavior using addresses and values, in order to allow cheating.

### License
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

### Offsets
```diff
+   0x331C50
+   0x868
+   0x94C
+   0x5578
+   0x54
+   0x120
```
Last Verified: **2023-08-22**

<img style="cursor: default;" align="right" width="100" height="100" src="img/logo.png">

### Features
- [x] Unlimited Sunflowers
- [x] Unlimited Coins
- [x] No Planting Delay
- [x] Tree of Wisdom Codes
- [x] Adjustable Tree Height

### Solution Tree
```
├───Hook (builds DLL file for PvZInjector)
├───PvZInjector (injects trainer to process)
└───PvZTrainer (external terminal CLI)
```

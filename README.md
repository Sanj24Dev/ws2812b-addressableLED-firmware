# ws2812b-addressableLED-firmware

## HOW TO USE THE LIBRARY
1. Initialize the TIMER for the DIN of Addressable LED  
  i.	Set the GPIO  
  ii.	Set the Period value to 89  
  iii.Add the DMA request for that channel and set the direction as Memory to peripheral
2. Use 'LED_Strip_Init' function to create variable for a strip
3. Use 'Set_LED' function to fill in values of rgb and brightness
4. Initialize the nextState to 0 before changing the function
5. Call the function in the following way inside the while loop
    ```
    now = HAL_GetTick();
    if(now - strip1->prev >= strip1->timeout){
        glow_alternate(strip1, c1, c2);
        Display(strip1);
        strip1->prev = HAL_GetTick();
    }
    ```

# TCM_drivers
A TCM driver demo using uart port.
  I use chip: DMT-FAC-CG4Q-2E;
  In my case, the chip is attached on the uart port(ttyS1).
usage:
  1. modefy the cross comiler path in make.sh
  2. run: ./make sh
  
 app/ including test code and cmmand stream for TCM test. 
 serial_dir/ including TCM driver via UART port
 tcm_driver/ including TCM cmmand header and cmmand data
 

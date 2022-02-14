----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date:    20:37:50 11/22/2013 
-- Design Name: 
-- Module Name:    cpu - Behavioral 
-- Project Name: 
-- Target Devices: 
-- Tool versions: 
-- Description: 
--
-- Dependencies: 
--
-- Revision: 
-- Revision 0.01 - File Created
-- Additional Comments: 
--
----------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity cpu is
    Port ( clk : in  STD_LOGIC;
           reset : in  STD_LOGIC;
           inst_addr : out  STD_LOGIC_VECTOR (31 downto 0);
           inst : in  STD_LOGIC_VECTOR (31 downto 0);
           data_addr : out  STD_LOGIC_VECTOR (31 downto 0);
           data : inout  STD_LOGIC_VECTOR (31 downto 0);
			  rd, wr: out std_logic);
end cpu;

architecture Behavioral of cpu is
-- types
  type reg_file_type is array (0 to 31) of unsigned(31 downto 0);
  
-- register file
  signal reg_file: reg_file_type;

-- Signals
  signal PC: unsigned(31 downto 0); -- program counter
  signal next_pc: unsigned(31 downto 0); -- program counter
  signal S: unsigned (31 downto 0); -- register contents for Rs
  signal T: unsigned (31 downto 0); -- register contents for Rt
  signal Imm: unsigned (31 downto 0); -- Sign Extended immediate operand
  signal ALU: unsigned (31 downto 0); -- ALU Result
  
-- instruction fields
  signal opcode: unsigned (5 downto 0); -- opcode
  signal R_s: unsigned   (4 downto 0);  -- address of register S
  signal R_t: unsigned   (4 downto 0);  -- address of register T
  signal R_d: unsigned   (4 downto 0);  -- address of register D (destination)
  signal funct: unsigned (5 downto 0);  -- function code
  
  -- opcodes
  constant Op_j:    unsigned(5 downto 0) := "000010"; -- jump
  constant Op_lw:   unsigned(5 downto 0) := "100011"; -- load word
  constant Op_sw:   unsigned(5 downto 0) := "101011"; -- store word
  constant Op_beq:  unsigned(5 downto 0) := "000100"; -- branch if equal
  constant Op_bne:  unsigned(5 downto 0) := "000101"; -- branch if not equal
  constant Op_addiu:unsigned(5 downto 0) := "001001"; -- addu immediate
  constant Op_andi: unsigned(5 downto 0) := "001100"; -- and immediate
  
  -- function codes
  constant Fn_addu: unsigned(5 downto 0) := "100001";
  constant Fn_subu: unsigned(5 downto 0) := "100011";
  constant Fn_and : unsigned(5 downto 0) := "100100";
  constant Fn_or  : unsigned(5 downto 0) := "100101";
  constant Fn_jr  : unsigned(5 downto 0) := "001000";

begin

-- Handle Program Counter (PC)
  process (clk, reset)
  begin
    if reset = '1' then
      PC <= (others=>'0');
    elsif rising_edge(clk) then
	    PC <= next_pc;
	 end if;
  end process;
  
  inst_addr <= std_logic_vector(PC);
  --
  -- Instruction memory takes inst_address and returns result in inst
  -- which is then aliased to opcode, Rs, Rt, funct, etc.
  --
  Opcode <= unsigned(inst (31 downto 26)); -- opcode
  R_s <= unsigned(inst (25 downto 21));    -- address of register S
  R_t <= unsigned(inst (20 downto 16));    -- address of register T
  R_d <= unsigned(inst (15 downto 11));    -- address of register to write back
  Funct <= unsigned(inst (5 downto 0));    -- Function code if opcode = 0
  
  -- get register data - register 0 always reads as 0
  S <= reg_file(to_integer(unsigned(R_s))) when R_s /= 0 else (others=>'0');
  T <= reg_file(to_integer(unsigned(R_t))) when R_t /= 0 else (others=>'0');
  Imm <= unsigned(resize(signed(inst(15 downto 0)), 32)); -- sign extend
  
  -- model the ALU
  -- default behavior is to add so load and store are handled automatically
  process (Opcode, Funct, S, T)
  begin
    case Opcode is
	    when "000000" => -- R format, use Function Code
		    case Funct is
		      when Fn_and => ALU <= S and T;
		      when Fn_or => ALU <= S or T;
		      when Fn_subu => ALU <= S - T;
		      when others => ALU <= S + T;
		    end case;
		  when "001100" => ALU <= S and imm;
		  when others =>  ALU <= S + Imm;
		end case;
	end process;
	
	-- model the memory
	wr <= '1' when Opcode = Op_sw else '0';
	rd <= '1' when Opcode = Op_lw else '0';
	data_addr <= std_logic_vector(ALU);
	data <= std_logic_vector(T) when Opcode = Op_sw else (others=>'Z');
	
	-- model the writeback to the register file
	process ( clk )
	begin
	  if rising_edge(clk) then
	    if Opcode = "000000" and Funct /= Fn_jr then
		    reg_file(to_integer(R_d)) <= ALU;
		 elsif Opcode = Op_lw then
		   reg_file(to_integer(R_t)) <= unsigned(data);
		 elsif Opcode = Op_andi or Opcode = Op_addiu then
			reg_file(to_integer(R_t)) <= ALU;
		 end if;
	  end if;
	end process;
	
  -- model the decoder for the next value of the PC
	process (PC, Inst, Opcode, S, T, Imm)
	  variable pc4: unsigned (31 downto 0);
	begin
	  pc4 := PC + 4;
	  if ( opcode = Op_j ) then
	    next_pc <= pc4(31 downto 28) & unsigned(Inst(25 downto 0)) & "00";
	  elsif opcode = "000000" and Funct = Fn_jr then
	    next_pc <= S;
	  elsif opcode = Op_beq then
		 if S = T then
			 next_pc <= pc4 + (imm(29 downto 0) & "00");
		 else
			 next_pc <= pc4;
		 end if;
	  elsif opcode = Op_bne then
		 if S /= T then
			next_pc <= pc4 + (imm(29 downto 0) & "00");
		 else
			next_pc <= pc4;
		 end if;
	  else
	    next_pc <= pc4;
	  end if;
	end process;
 
end Behavioral;


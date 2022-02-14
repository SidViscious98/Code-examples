----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date:    14:08:30 10/08/2019 
-- Design Name: 
-- Module Name:    video - Behavioral 
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

-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx primitives in this code.
library UNISIM;
use UNISIM.VComponents.all;

entity video is
	Port(clk: in std_logic;
		  rst: in std_logic;
		  hsync: out std_logic;
		  vsync: out std_logic;
		  R, G, B: out std_logic);
end video;

architecture Behavioral of video is
signal hcount: integer range 0 to 1343;
signal vcount: integer range 0 to 805;
signal hblank, vblank, blanking, hstart: std_logic;
signal row: std_logic_vector(9 downto 0);
signal count: unsigned(2 downto 0);


begin
process(clk, rst)
begin
	if rst = '1' then 
		hcount <= 0;
	elsif rising_edge(clk) then
		if hcount = 1047 then
			hsync <= '0';
		elsif hcount = 1183 then
			hsync <= '1';
		end if;
		if hcount = 1023 then
			hblank <= '1';
		elsif hcount = 1343 then
			hblank <= '0';
		end if;
		if hcount = 1343 then
			hcount <= 0;
		else
			hcount <= hcount + 1;
		end if;
	end if;
end process;

process (clk, rst)
begin
	if rst = '1' then
		vcount <= 0;
	elsif rising_edge(clk) then
		if hcount = 1100 then
			if vcount = 770 then
				vsync <= '0';
			elsif vcount = 776 then
				vsync <= '1';
			end if;
			if vcount = 766 then
				vblank <= '1';
			elsif vcount = 805 then
				vblank <= '0';
			end if;
			if vcount = 805 then
				vcount <= 0;
			else
				vcount <= vcount + 1;
			end if;
		end if;
	end if;
end process;

blanking <= hblank or vblank;
hstart <= '1' when hcount = 1343 else '0';
--vstart <= '1' when vcount = 805 else '0';
row <= std_logic_vector(unsigned(to_unsigned(vcount,10)));
--column <= std_logic_vector(unsigned(to_unsigned(hcount,11)));

process (clk, hstart)
begin
if rising_edge(clk) then
	if hstart = '1' then
			count <= unsigned(row(2 downto 0));
		else 
			count <= count + 1;
		end if;
	end if;
end process;
R <= count(0) and not blanking;
G <= count(1) and not blanking;
B <= count(2) and not blanking;


end Behavioral;


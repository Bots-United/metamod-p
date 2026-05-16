/* Align hot-path data sections to cache line boundary */
SECTIONS
{
  .data.hot_data : ALIGN(64)
  {
    *(.data.hot_data)
  }
  .data.hot_rodata : ALIGN(4)
  {
    *(.data.hot_rodata)
  }
}
INSERT AFTER .data;

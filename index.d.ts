 /**
   * Extracts the icon from a file (like `.exe` or `.lnk`) as a raw BGRA bitmap buffer.
   * The returned buffer contains 32-bit pixel data (BGRA, top-down row order).
   * 
   * @param inputPath Absolute path to the target file.
   * @param size Desired width and height of the icon.
   * @returns Buffer with raw image data.
   */
  export function getIconBuffer(inputPath: string, size: number): Buffer;

  /**
   * Extracts a thumbnail from a file using IThumbnailProvider, as a raw BGRA bitmap buffer.
   * The returned buffer contains 32-bit pixel data (BGRA, top-down row order).
   * 
   * @param inputPath Absolute path to the target file.
   * @param size Desired width and height of the thumbnail.
   * @returns Buffer with raw image data.
   */
  export function getThumbnailBuffer(inputPath: string, size: number): Buffer;

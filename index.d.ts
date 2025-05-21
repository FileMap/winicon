/**
 * Extracts the associated windows icon from a file and saves it as a PNG.
 * @param inputPath Absolute path to the file (e.g., `.exe`, `.lnk`)
 * @param outputPath Absolute path to save the PNG
 * @param size Size of the icon (e.g., 256)
 */
export function getIcon(inputPath: string, outputPath: string, size: number): void;

/**
 * Extracts a thumbnail provided by applications using IThumbnailProvider and saves it as a PNG.
 * @param inputPath Absolute path to the file (e.g., `.pdf`, `.docx`)
 * @param outputPath Absolute path to save the PNG
 * @param size Size of the thumbnail
 */
export function getThumbnail(inputPath: string, outputPath: string, size: number): void;
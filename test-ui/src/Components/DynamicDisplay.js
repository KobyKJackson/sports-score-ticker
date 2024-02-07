import React from 'react';
import { Box, Typography, Paper } from '@mui/material';
import contrastRatio from '../Utilities';

const DynamicDisplay = ({ data }) => {
    // Calculate height based on location
    const calculateHeight = (location) => {
        // Determine the number of rows the item should span
        const numberOfRows = location.length;
        // Calculate the height by multiplying the number of rows by the height per row
        return `${numberOfRows * 16}px`;
    };
    const renderDataItem = (item, index) => {
        const height = item.location ? calculateHeight(item.location) : 'auto';
        let style = {
            height,
            display: 'flex',
            alignItems: 'center',
        };

        switch (item.type) {
            case 'image':
                return (
                    <img key={index} src={item.data} alt="" style={{ ...style, width: 'auto', height }} />
                );
            case 'text':
                let color = 'inherit'
                if (item.color) {
                    if (contrastRatio('#' + item.color, '#121212') >= 2) {
                        color = '#' + item.color
                    }
                    else {
                        color = '#' + item.altColor
                    }
                }
                const textStyle = {
                    ...style,
                    flexGrow: 1,
                    fontSize: height,
                    color: color
                };
                return (
                    <Typography key={index} style={textStyle}>
                        {item.data}
                    </Typography>
                );
            case 'multi':
                return item.data.map((subItem, subIndex) => renderDataItem(subItem, `multi-${index}-${subIndex}`));
            default:
                return null;
        }
    };

    return (
        <Paper elevation={3} sx={{ display: 'flex', overflow: 'auto', whiteSpace: 'nowrap', p: 1 }}>
            {data.map((item, index) => (
                <Box key={index} sx={{ display: 'inline-flex', flexDirection: 'column', marginRight: '8px' }}>
                    {renderDataItem(item, index)}
                </Box>
            ))}
        </Paper>
    );
};

export default DynamicDisplay;
